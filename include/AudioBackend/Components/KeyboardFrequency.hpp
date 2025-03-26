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
};

