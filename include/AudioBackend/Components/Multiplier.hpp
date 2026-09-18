#pragma once

#include "AudioComponent.hpp"

struct Multiplier : public AudioComponent {
	enum Inputs { inputA, inputB };

	Multiplier() : AudioComponent() { inputs.resize(2); componentName = "Multiplier"; }

	std::shared_ptr<AudioComponent> clone() const override {
		return std::make_shared<Multiplier>();
	}

	double process(const AudioInfos& audioInfos) override
	{
		double valueA = getInputsValue(inputA, audioInfos);
		double valueB = getInputsValue(inputB, audioInfos);
		return valueA * valueB;
	}
};
