#pragma once

#include "AudioComponent.hpp"
#include "audio_backend.hpp"

struct Multiplier : public AudioComponent {
	std::vector<AudioComponent*> inputsA;
	std::vector<AudioComponent*> inputsB;

	Components getInputs() override
	{
		return combineVectorsToForwardList(inputsA, inputsB);
	}

	void clearInputs() override
	{
		clearVectors(inputsA, inputsB);
	}

	void addInput(const std::string& inputName, AudioComponent* input) override
	{
		// [TODO] remove those "> "
		if (inputName == "> input A") inputsA.push_back(input);
		else if (inputName == "> input B") inputsB.push_back(input);
		else assert(0 && "[Oscillator node] unknown input");
	}

	double process(std::vector<MidiInfo>& keyPressed, int currentKey = 0) override
	{
		double valueA = getInputsValue(inputsA, keyPressed, currentKey);
		double valueB = getInputsValue(inputsB, keyPressed, currentKey);
		return valueA * valueB;
	}
};
