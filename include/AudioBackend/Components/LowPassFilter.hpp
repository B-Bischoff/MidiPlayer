#pragma once

#include <algorithm>
#include "AudioComponent.hpp"
#include "audio_backend.hpp"

struct LowPassFilter : public AudioComponent {
	enum Inputs { input, alpha };

	double state = 0.0;

	LowPassFilter() : AudioComponent() { inputs.resize(2); componentName = "LowPassFilter"; }

	double process(PipelineInfo& info) override
	{
		double alphaValue = std::clamp(getInputsValue(alpha, info), 0.0, 1.0);
		double signalValue = getInputsValue(input, info);

		state = alphaValue * signalValue + (1.0 - alphaValue) * state;

		return state;
	}
};
