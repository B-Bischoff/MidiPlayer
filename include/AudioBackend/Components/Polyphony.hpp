#pragma once

#include <vector>
#include "AudioComponent.hpp"
#include "MidiSourceComponent.hpp"
#include "AudioBackend/VoiceContext.hpp"

struct Polyphony : public AudioComponent {
	enum Inputs { audioTemplate };

	static constexpr int DEFAULT_MAX_VOICES = 8;

	struct Voice {
		int noteId = -1;
		bool active = false;
		bool releasing = false;
		int releaseSamples = 0;
		unsigned int generation = 0; // Incremented on each voice assignment
		MidiInfo info = {};
		VoiceContext context;
		std::shared_ptr<AudioComponent> graphRoot;
	};

	int maxVoices;
	bool hasEnvelope = false; // True if sub-graph contains an ADSR
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

		auto& templateRoot = inputs[audioTemplate].front();
		hasEnvelope = containsADSR(templateRoot);

		for (Voice& v : voices)
		{
			v.graphRoot = templateRoot->deepClone();
			v.context.clear();
			v.active = false;
			v.releasing = false;
			v.noteId = -1;
		}
	}

	void assignVoice(int note, int velocity)
	{
		// Re-trigger if note already assigned
		for (Voice& v : voices)
		{
			if ((v.active || v.releasing) && v.noteId == note)
			{
				v.active = true;
				v.releasing = false;
				v.info = { note, velocity, true };
				v.generation++;
				return;
			}
		}

		// Find free voice
		for (Voice& v : voices)
		{
			if (!v.active && !v.releasing)
			{
				v.active = true;
				v.releasing = false;
				v.noteId = note;
				v.info = { note, velocity, true };
				v.generation++;
				v.context.clear();
				return;
			}
		}

		// Voice stealing: replace first active voice
		for (Voice& v : voices)
		{
			if (v.active)
			{
				v.noteId = note;
				v.info = { note, velocity, true };
				v.releasing = false;
				v.generation++;
				v.context.clear();
				return;
			}
		}
	}

	void releaseVoice(int note)
	{
		for (Voice& v : voices)
		{
			if (v.active && v.noteId == note)
			{
				v.active = false;
				if (hasEnvelope)
				{
					// Let ADSR release tail play out
					v.releasing = true;
					v.releaseSamples = 0;
				}
				else
				{
					// No envelope — stop immediately
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
		v.releaseSamples = 0;
		v.noteId = -1;
	}

	double process(const AudioInfos& audioInfos, std::vector<MidiInfo>& keyPressed, int currentKey = 0) override
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
			std::vector<MidiInfo> singleNote = { v.info };
			double voiceValue = v.graphRoot->process(audioInfos, singleNote, 0);
			sum += voiceValue;

			// Deactivate releasing voices that have gone silent
			if (v.releasing)
			{
				// Count consecutive near-silent samples to avoid false positives
				// from oscillator zero-crossings
				if (std::abs(voiceValue) < 1e-6)
					v.releaseSamples++;
				else
					v.releaseSamples = 0;

				// Require sustained silence (~5ms at 44100Hz) before deactivating
				if (v.releaseSamples > 220)
					deactivateVoice(v);
			}

			activeVoiceContext = prevContext;
		}

		return sum;
	}

private:
	static bool containsADSR(const std::shared_ptr<AudioComponent>& node)
	{
		if (!node) return false;
		if (node->componentName == "ADSR") return true;
		for (auto& slot : node->inputs)
			for (auto& child : slot)
				if (containsADSR(child)) return true;
		return false;
	}
};
