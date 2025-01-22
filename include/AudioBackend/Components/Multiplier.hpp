#pragma once

#include "AudioComponent.hpp"
#include "audio_backend.hpp"

struct Multiplier : public AudioComponent {
	enum Inputs { inputA, inputB };

	Multiplier() : AudioComponent() { inputs.resize(2); }

	double process(std::vector<MidiInfo>& keyPressed, int currentKey = 0) override
	{
		double valueA = getInputsValue(inputA, keyPressed, currentKey);
		double valueB = getInputsValue(inputB, keyPressed, currentKey);
		return valueA * valueB;
	}
};
