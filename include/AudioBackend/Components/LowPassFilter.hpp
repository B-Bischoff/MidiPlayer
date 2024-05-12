#pragma once

#include "AudioComponent.hpp"
#include "audio_backend.hpp"

struct LowPassFilter : public AudioComponent {
	std::vector<AudioComponent*> signals;
	std::vector<AudioComponent*> alpha;

	double state = 0.0;

	Components getInputs() override
	{
		return combineVectorsToForwardList(signals, alpha);
	}

	void clearInputs() override
	{
		clearVectors(signals, alpha);
	}

	void addInput(const std::string& inputName, AudioComponent* input) override
	{
		if (inputName == "> signal") signals.push_back(input);
		else if (inputName == "> alpha") alpha.push_back(input);
		else assert(0 && "[LowPassFilter] unknown input");
	}

	double process(std::vector<MidiInfo>& keyPressed, int currentKey = 0) override
	{
		double alphaValue = getInputsValue(alpha, keyPressed, currentKey);
		double signalValue = getInputsValue(signals, keyPressed, currentKey);

		state = alphaValue * signalValue + (1.0 - alphaValue) * state;

		return state;
	}
};
