#pragma once

#include <cmath>
#include "AudioComponent.hpp"

struct Overdrive : public AudioComponent {
	enum Inputs { input, drive };

	Overdrive() : AudioComponent() { inputs.resize(2); componentName = "Overdrive"; }

	double process(PipelineInfo& info) override
	{
		const double inputValue = getInputsValue(input, info);
		const double driveValue = getInputsValue(drive, info);

		return std::tanh(inputValue * driveValue);
	}
};
