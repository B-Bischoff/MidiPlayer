#pragma once

#include "MidiSourceComponent.hpp"

struct KeyboardFrequency : public MidiSourceComponent {
	static unsigned int keyIndex;

	KeyboardFrequency() : MidiSourceComponent() { componentName = "KeyboardFrequency"; }

	std::shared_ptr<AudioComponent> clone() const override {
		return std::make_shared<KeyboardFrequency>();
	}

	std::vector<MidiInfo> processMidi(const AudioInfos& audioInfos) override
	{
		// Keyboard input is fed externally via the keyPressed parameter
		return {};
	}

	double process(const AudioInfos& audioInfos, std::vector<MidiInfo>& keyPressed, int currentKey = 0) override
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
