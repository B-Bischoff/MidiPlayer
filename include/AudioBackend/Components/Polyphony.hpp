#pragma once

#include "AudioComponent.hpp"

struct Polyphony : public AudioComponent {
	Polyphony() : AudioComponent() { componentName = "Polyphony"; }

	double process(const AudioInfos& audioInfos, std::vector<MidiInfo>& keyPressed, int currentKey = 0) override
	{
		return 0.0;
	}
};
