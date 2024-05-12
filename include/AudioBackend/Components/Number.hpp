#pragma once

#include "AudioComponent.hpp"

struct Number : public AudioComponent {
	float number;

	double process(std::vector<MidiInfo>& keyPressed, int currentKey = 0) override
	{
		return number;
	}
};
