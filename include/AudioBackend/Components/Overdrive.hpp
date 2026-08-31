#pragma once

#include <cmath>
#include "AudioComponent.hpp"

struct Overdrive : public AudioComponent {
	enum Inputs { input, drive };

	Overdrive() : AudioComponent() { inputs.resize(2); componentName = "Overdrive"; }

	std::shared_ptr<AudioComponent> clone() const override {
		return std::make_shared<Overdrive>();
	}

	double process(const AudioInfos& audioInfos, std::vector<MidiInfo>& keyPressed, int currentKey = 0) override
	{
		const double inputValue = getInputsValue(input, audioInfos, keyPressed, currentKey);
		const double driveValue = getInputsValue(drive, audioInfos, keyPressed, currentKey);

		return std::tanh(inputValue * driveValue);
	}
};
