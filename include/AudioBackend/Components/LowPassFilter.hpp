#pragma once

#include <algorithm>
#include "AudioComponent.hpp"
#include "audio_backend.hpp"

struct LowPassFilter : public AudioComponent {
	enum Inputs { input, cutoff, resonance };

	double low = 0.0;
	double band = 0.0;

	LowPassFilter() : AudioComponent() { inputs.resize(3); componentName = "LowPassFilter"; }

	double process(std::vector<MidiInfo>& keyPressed, int currentKey = 0) override
	{
		double cutoffValue = std::clamp(getInputsValue(cutoff, keyPressed, currentKey), 0.01, 0.99);
		double resonanceValue = std::clamp(getInputsValue(resonance, keyPressed, currentKey), 0.0, 4.0);
		double inputValue = getInputsValue(input, keyPressed, currentKey);

		double high = inputValue - low - resonanceValue * band;
		band += cutoffValue * high;
		low += cutoffValue * band;

		return low;
	}
};
