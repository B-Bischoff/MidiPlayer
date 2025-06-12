#pragma once

#include "AudioComponent.hpp"

struct Number : public AudioComponent {
	float number;

	Number() : AudioComponent() { componentName = "Number"; }

	double process(PipelineInfo& info) override
	{
		return number;
	}
};
