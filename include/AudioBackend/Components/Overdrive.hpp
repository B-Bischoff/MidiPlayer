#pragma once

#include <cmath>
#include "AudioComponent.hpp"

struct Overdrive : public AudioComponent {
	enum Inputs { input, drive };

	Overdrive() : AudioComponent() { inputs.resize(2); componentName = "Overdrive"; }

	double process(std::vector<MidiInfo>& keyPressed, int currentKey = 0) override
	{
		const double inputValue = getInputsValue(input, keyPressed, currentKey);
		const double driveValue = getInputsValue(drive, keyPressed, currentKey);

		return std::tanh(inputValue * driveValue);
	}
};
