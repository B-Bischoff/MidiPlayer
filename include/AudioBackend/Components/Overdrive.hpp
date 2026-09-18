#pragma once

#include <cmath>
#include "AudioComponent.hpp"

struct Overdrive : public AudioComponent {
	enum Inputs { input, drive };

	Overdrive() : AudioComponent() { inputs.resize(2); componentName = "Overdrive"; }

	std::shared_ptr<AudioComponent> clone() const override {
		return std::make_shared<Overdrive>();
	}

	void computeOutputs(const AudioInfos& audioInfos) override
	{
		const double inputValue = getInputsValue(input, audioInfos);
		const double driveValue = getInputsValue(drive, audioInfos);

		outputValues[0] = std::tanh(inputValue * driveValue);
	}
};
