#pragma once

#include <algorithm>
#include "AudioComponent.hpp"

struct HighPassFilter : public AudioComponent {
	enum Inputs { input, alpha };

	double state = 0.0;

	HighPassFilter() : AudioComponent() { inputs.resize(2); componentName = "HighPassFilter"; }

	double process(const AudioInfos& audioInfos, std::vector<MidiInfo>& keyPressed, int currentKey = 0) override
	{
		double alphaValue = std::clamp(getInputsValue(alpha, audioInfos, keyPressed, currentKey), 0.0, 1.0);
		double signalValue = getInputsValue(input, audioInfos, keyPressed, currentKey);

		state = alphaValue * signalValue + (1.0 - alphaValue) * state;

		return signalValue - state; // This is the only difference with a low pass filter
	}
};
