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
		// Pass-through for now — voice context is not wired into the audio pull yet
		if (inputs[audioTemplate].empty())
			return 0.0;
		return getInputsValue(audioTemplate, audioInfos, keyPressed, currentKey);
	}
};
