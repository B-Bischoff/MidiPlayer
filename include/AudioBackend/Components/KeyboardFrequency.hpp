#pragma once

#include "MidiSourceComponent.hpp"
#include "AudioBackend/VoiceContext.hpp"

struct KeyboardFrequency : public MidiSourceComponent {
	static unsigned int keyIndex;

	KeyboardFrequency() : MidiSourceComponent() { componentName = "KeyboardFrequency"; }

	std::shared_ptr<AudioComponent> clone() const override {
		return std::make_shared<KeyboardFrequency>();
	}

	std::vector<MidiInfo> processMidi(const AudioInfos& audioInfos) override
	{
		return {};
	}

	double process(const AudioInfos& audioInfos, std::vector<MidiInfo>& keyPressed, int currentKey = 0) override
	{
		// When inside a polyphonic voice, use the voice context note
		if (activeVoiceContext && activeVoiceContext->noteInfo.keyIndex > 0)
			return pianoKeyFrequency(activeVoiceContext->noteInfo.keyIndex);

		// Fallback: direct keyPressed (non-polyphonic path)
		if (!keyPressed.size())
			return 0.0;
		return pianoKeyFrequency(keyPressed[currentKey].keyIndex);
	}

	double pianoKeyFrequency(int keyId)
	{
		double A4Frequency = 440.0;
		int keysDifference = keyId - 69;
		double semitoneRatio = pow(2.0, 1.0/12.0);
		return A4Frequency * pow(semitoneRatio, keysDifference);
	}
};
