#pragma once

#include "AudioComponent.hpp"

struct Number : public AudioComponent {
	float number;

	Number() : AudioComponent() { componentName = "Number"; }

	double process(const AudioInfos& audioInfos, std::vector<MidiInfo>& keyPressed, int currentKey = 0) override
	{
		return number;
	}
};
