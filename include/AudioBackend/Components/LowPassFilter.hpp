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
		double cutoffValue = std::clamp(getInputsValue(cutoff, audioInfos, keyPressed, currentKey), 0.01, 0.99);
		double resonanceValue = std::clamp(getInputsValue(resonance, audioInfos, keyPressed, currentKey), 0.05, 4.0);
		double inputValue = getInputsValue(input, audioInfos, keyPressed, currentKey);

		double high = inputValue - low - resonanceValue * band;
		band += cutoffValue * high;
		low += cutoffValue * band;

		return low;
	}
};
