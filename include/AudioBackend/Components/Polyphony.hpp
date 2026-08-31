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
		MidiInfo info = {};
		VoiceContext context;
		std::shared_ptr<AudioComponent> graphRoot; // Cloned sub-graph for this voice
	};

	int maxVoices;
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
				v.releasing = true;
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

	double process(const AudioInfos& audioInfos, std::vector<MidiInfo>& keyPressed, int currentKey = 0) override
	{
		// Only process voice management once per sample (currentKey == 0)
		if (currentKey == 0)
			updateVoices(keyPressed);

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

			// Pull from the cloned sub-graph (single voice, currentKey=0)
			std::vector<MidiInfo> singleNote = { v.info };
			double voiceValue = v.graphRoot->process(audioInfos, singleNote, 0);
			sum += voiceValue;

			// Deactivate releasing voices that have gone silent
			if (v.releasing && std::abs(voiceValue) < 1e-10)
				deactivateVoice(v);

			activeVoiceContext = prevContext;
		}

		return sum;
	}

private:
	std::vector<int> _previousNotes; // Track pressed notes for NoteOff detection

	void updateVoices(const std::vector<MidiInfo>& keyPressed)
	{
		// NoteOn: assign voices for newly pressed keys
		for (const MidiInfo& key : keyPressed)
			assignVoice(key.keyIndex, key.velocity);

		// NoteOff: release voices for keys no longer pressed
		for (int prevNote : _previousNotes)
		{
			bool stillPressed = false;
			for (const MidiInfo& key : keyPressed)
			{
				if (key.keyIndex == prevNote)
				{
					stillPressed = true;
					break;
				}
			}
			if (!stillPressed)
				releaseVoice(prevNote);
		}

		// Update tracking
		_previousNotes.clear();
		for (const MidiInfo& key : keyPressed)
			_previousNotes.push_back(key.keyIndex);
	}
};
