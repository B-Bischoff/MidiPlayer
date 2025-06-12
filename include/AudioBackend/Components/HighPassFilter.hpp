#pragma once

#include <algorithm>
#include "AudioComponent.hpp"

struct HighPassFilter : public AudioComponent {
	enum Inputs { input, alpha };

	double state = 0.0;

	HighPassFilter() : AudioComponent() { inputs.resize(2); componentName = "HighPassFilter"; }

	double process(PipelineInfo& info) override
	{
		double alphaValue = std::clamp(getInputsValue(alpha, info), 0.0, 1.0);
		double signalValue = getInputsValue(input, info);

		state = alphaValue * signalValue + (1.0 - alphaValue) * state;

		return signalValue - state; // This is the only difference with a low pass filter
	}
};
