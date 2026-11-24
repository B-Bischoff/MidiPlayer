#pragma once

#include <vector>
#include "AudioComponent.hpp"
#include "MidiSourceComponent.hpp"
#include "AudioBackend/VoiceContext.hpp"

struct Polyphony : public AudioComponent {
	enum Inputs { audioTemplate };

	static constexpr int DEFAULT_MAX_VOICES = 8;

	// Safety net: a releasing voice is freed after this long even if its
	// release-tail components never report finished
	static constexpr double MAX_RELEASE_SECONDS = 10.0;

	struct Voice {
		int noteId = -1;
		bool active = false;
		bool releasing = false;
		double releaseStartTime = 0.0;
		unsigned long assignOrder = 0; // Higher = assigned more recently
		unsigned int generation = 0; // Incremented on each voice assignment
		MidiInfo info = {};
		VoiceContext context;
		std::shared_ptr<AudioComponent> graphRoot;

		// Components of graphRoot with a release stage (non-owning, kept alive by graphRoot).
		// A released voice is freed once all of them have finished their release.
		std::vector<AudioComponent*> releaseNodes;
	};

	int maxVoices;
	int templateOutputIndex = 0; // Which output of the template sub-graph root each voice should read
	std::vector<Voice> voices;
	MidiSourceComponent* midiSource = nullptr;

	Polyphony(int maxVoices = DEFAULT_MAX_VOICES)
		: AudioComponent(), maxVoices(maxVoices)
	{
		inputs.resize(1);
		componentName = "Polyphony";
		voices.resize(maxVoices);
	}

	std::shared_ptr<AudioComponent> clone() const override {
		return std::make_shared<Polyphony>(maxVoices);
	}

	// Deep-clone the audio template sub-graph for each voice.
	// Called by the Compiler after wiring the template into inputs[audioTemplate].
	void compileVoices()
	{
		if (inputs[audioTemplate].empty())
			return;

		templateOutputIndex = inputs[audioTemplate].front().outputIndex;

		for (Voice& v : voices)
		{
			buildVoiceGraph(v);
			v.context.clear();
			deactivateVoice(v);
		}
	}

	void assignVoice(int note, int velocity)
	{
		// Re-trigger if note already assigned
		for (Voice& v : voices)
		{
			if ((v.active || v.releasing) && v.noteId == note)
			{
				startVoice(v, note, velocity, false);
				return;
			}
		}

		// Find free voice
		for (Voice& v : voices)
		{
			if (!v.active && !v.releasing)
			{
				startVoice(v, note, velocity, true);
				return;
			}
		}

		// Voice stealing: prefer the oldest release tail, then the oldest held note
		Voice* stolen = findOldest([](const Voice& v) { return v.releasing; }, &Voice::releaseStartTime);
		if (!stolen)
			stolen = findOldest([](const Voice& v) { return v.active; }, &Voice::assignOrder);

		if (stolen)
			startVoice(*stolen, note, velocity, true);
	}

	void releaseVoice(int note)
	{
		for (Voice& v : voices)
		{
			if (v.active && v.noteId == note)
			{
				v.active = false;
				if (!v.releaseNodes.empty())
				{
					// Let the release tail play out
					v.releasing = true;
					v.releaseStartTime = time;
				}
				else
				{
					// No release stage — stop immediately
					deactivateVoice(v);
				}
				return;
			}
		}
	}

	void deactivateVoice(Voice& v)
	{
		v.active = false;
		v.releasing = false;
		v.noteId = -1;
	}

	void computeOutputs(const AudioInfos& audioInfos) override
	{
		// Only process MIDI events once per sample (skip on second channel in stereo)
		if (midiSource && audioInfos.currentChannel == 0)
		{
			auto events = midiSource->processMidi(audioInfos);
			for (const MidiEvent& e : events)
			{
				if (e.type == MidiEvent::NoteOn)
					assignVoice(e.note, e.velocity);
				else if (e.type == MidiEvent::NoteOff)
					releaseVoice(e.note);
			}
		}

		// Pull audio from each active voice's cloned sub-graph
		double sum = 0.0;
		for (Voice& v : voices)
		{
			if ((!v.active && !v.releasing) || !v.graphRoot)
				continue;

			// Set voice context so downstream nodes use per-voice state
			VoiceContext* prevContext = activeVoiceContext;
			activeVoiceContext = &v.context;

			v.context.noteInfo = v.info;
			v.context.releasing = v.releasing;
			v.context.generation = v.generation;

			// Pull from the cloned sub-graph (single voice)
			sum += v.graphRoot->getOutput(audioInfos, templateOutputIndex);

			// Free releasing voices once every release stage has ended
			// (their state was just updated by the getOutput() call above)
			if (v.releasing && (releaseFinished(v) || time - v.releaseStartTime > MAX_RELEASE_SECONDS))
				deactivateVoice(v);

			activeVoiceContext = prevContext;
		}

		outputValues[0] = sum;
	}

private:
	unsigned long _assignCounter = 0;

	void startVoice(Voice& v, int note, int velocity, bool clearContext)
	{
		v.active = true;
		v.releasing = false;
		v.noteId = note;
		v.info = { note, velocity, true };
		v.generation++;
		v.assignOrder = ++_assignCounter;
		if (clearContext)
			v.context.clear();
	}

	void buildVoiceGraph(Voice& v)
	{
		v.graphRoot = inputs[audioTemplate].front().component->deepClone();
		v.releaseNodes.clear();
		collectReleaseNodes(v.graphRoot, v.releaseNodes);
	}

	// Oldest voice matching `eligible`, compared on `age` (smaller = older), or nullptr
	template<typename Eligible, typename Age>
	Voice* findOldest(Eligible eligible, Age Voice::* age)
	{
		Voice* oldest = nullptr;
		for (Voice& v : voices)
		{
			if (eligible(v) && (!oldest || v.*age < oldest->*age))
				oldest = &v;
		}
		return oldest;
	}

	static bool releaseFinished(const Voice& v)
	{
		for (const AudioComponent* node : v.releaseNodes)
		{
			if (!node->isReleaseFinished())
				return false;
		}
		return true;
	}

	static void collectReleaseNodes(const std::shared_ptr<AudioComponent>& node, std::vector<AudioComponent*>& result)
	{
		if (!node) return;
		if (node->hasReleaseTail())
			result.push_back(node.get());
		for (auto& slot : node->inputs)
			for (auto& child : slot)
				collectReleaseNodes(child.component, result);
	}
};
