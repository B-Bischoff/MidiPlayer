#pragma once

#include <algorithm>
#include "AudioComponent.hpp"
#include "audio_backend.hpp"

struct LowPassFilter : public AudioComponent {
	enum Inputs { input, cutoff, resonance };

	double low = 0.0;
	double band = 0.0;

	LowPassFilter() : AudioComponent() { inputs.resize(3); componentName = "LowPassFilter"; }

	double process(const AudioInfos& audioInfos, std::vector<MidiInfo>& keyPressed, int currentKey = 0) override
	{
		const double cutoffValue = std::clamp(getInputsValue(cutoff, audioInfos, keyPressed, currentKey), 0.01, 0.99);
		const double resonanceValue = std::clamp(getInputsValue(resonance, audioInfos, keyPressed, currentKey), 0.00, 0.95);
		const double inputValue = getInputsValue(input, audioInfos, keyPressed, currentKey);

		const double high = inputValue - low - (1.0 - resonanceValue) * band;
		band += cutoffValue * high;
		low += cutoffValue * band;

		return low;
	}
};
