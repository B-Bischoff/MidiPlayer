#pragma once

#include <algorithm>
#include "AudioComponent.hpp"

struct LowPassFilter : public AudioComponent {
	enum Inputs { input, cutoff, resonance };

	double low = 0.0;
	double band = 0.0;

	LowPassFilter() : AudioComponent() { inputs.resize(3); componentName = "LowPassFilter"; }

	std::shared_ptr<AudioComponent> clone() const override {
		return std::make_shared<LowPassFilter>();
	}

	double process(const AudioInfos& audioInfos) override
	{
		const double cutoffValue = std::clamp(getInputsValue(cutoff, audioInfos), 0.01, 0.99);
		const double resonanceValue = std::clamp(getInputsValue(resonance, audioInfos), 0.00, 0.95);
		const double inputValue = getInputsValue(input, audioInfos);

		const double high = inputValue - low - (1.0 - resonanceValue) * band;
		band += cutoffValue * high;
		low += cutoffValue * band;

		return low;
	}
};
