#pragma once

#include "AudioComponent.hpp"
#include "audio_backend.hpp"

struct Multiplier : public AudioComponent {
	enum Inputs { inputA, inputB };

	Multiplier() : AudioComponent() { inputs.resize(2); componentName = "Multiplier"; }

	double process(const AudioInfos& audioInfos, std::vector<MidiInfo>& keyPressed, int currentKey = 0) override
	{
		double valueA = getInputsValue(inputA, audioInfos, keyPressed, currentKey);
		double valueB = getInputsValue(inputB, audioInfos, keyPressed, currentKey);
		return valueA * valueB;
	}
};
