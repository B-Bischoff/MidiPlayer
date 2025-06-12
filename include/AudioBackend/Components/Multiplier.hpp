#pragma once

#include "AudioComponent.hpp"
#include "audio_backend.hpp"

struct Multiplier : public AudioComponent {
	enum Inputs { inputA, inputB };

	Multiplier() : AudioComponent() { inputs.resize(2); componentName = "Multiplier"; }

	double process(PipelineInfo& info) override
	{
		double valueA = getInputsValue(inputA, info);
		double valueB = getInputsValue(inputB, info);
		return valueA * valueB;
	}
};
