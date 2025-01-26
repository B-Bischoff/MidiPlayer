#pragma once

#include "AudioComponent.hpp"
#include "audio_backend.hpp"

struct LowPassFilter : public AudioComponent {
	enum Inputs { input, alpha };

	double state = 0.0;

	LowPassFilter() : AudioComponent() { inputs.resize(2); componentName = "LowPassFilter"; }

	double process(std::vector<MidiInfo>& keyPressed, int currentKey = 0) override
	{
		double alphaValue = getInputsValue(alpha, keyPressed, currentKey);
		double signalValue = getInputsValue(input, keyPressed, currentKey);

		state = alphaValue * signalValue + (1.0 - alphaValue) * state;

		return state;
	}
};
