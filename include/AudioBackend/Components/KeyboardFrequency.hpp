#pragma once

#include "AudioComponent.hpp"
#include "audio_backend.hpp"

struct KeyboardFrequency : public AudioComponent {
	static unsigned int keyIndex;

	KeyboardFrequency() : AudioComponent() { componentName = "KeyboardFrequency"; }

	double process(std::vector<MidiInfo>& keyPressed, int currentKey = 0) override
	{
		if (!keyPressed.size())
			return 0.0;

		return pianoKeyFrequency(keyPressed[currentKey].keyIndex);
	}

	double pianoKeyFrequency(int keyId)
	{
		// Frequency of key A4 (A440) is 440 Hz
		double A4Frequency = 440.0;

		// Number of keys from A4 to the given key
		int keysDifference = keyId - 69;

		// Frequency multiplier for each semitone
		double semitoneRatio = pow(2.0, 1.0/12.0);

		// Calculate the frequency of the given key
		double frequency = A4Frequency * pow(semitoneRatio, keysDifference);

		//std::cout << keyId << " " << frequency << std::endl;

		return frequency;
	}
};
