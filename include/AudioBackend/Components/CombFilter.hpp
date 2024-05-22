#pragma once

#include "AudioComponent.hpp"
#include "audio_backend.hpp"

struct CombFilter : public AudioComponent {
	std::vector<AudioComponent*> input;
	std::vector<AudioComponent*> delaySamples;
	std::vector<AudioComponent*> feedback;

	std::vector<double> delayBuffer;
	int bufferIndex = 0;

	Components getInputs() override
	{
		return combineVectorsToForwardList(input, delaySamples, feedback);
	}

	void clearInputs() override
	{
		clearVectors(input, delaySamples, feedback);
	}

	void addInput(const std::string& inputName, AudioComponent* input) override
	{
		if (inputName == "> input") this->input.push_back(input);
		else if (inputName == "> delay samples") delaySamples.push_back(input);
		else if (inputName ==  "> feedback") feedback.push_back(input);
		else assert(0 && "[CombFilter] unknown input");
	}

	double process(std::vector<MidiInfo>& keyPressed, int currentKey = 0) override
	{
		int delaySamplesValue = static_cast<int>(getInputsValue(delaySamples, keyPressed, currentKey));
		double feedbackValue = getInputsValue(feedback, keyPressed, currentKey);
		if (feedbackValue > 1.0) feedbackValue = 1.0;
		double inputValue = getInputsValue(input, keyPressed, currentKey);

		if (delaySamplesValue > 0 && delaySamplesValue != delayBuffer.size())
		{
			delayBuffer.resize(delaySamplesValue);
			if (bufferIndex >= delayBuffer.size())
				bufferIndex = 0;
		}

		if (delayBuffer.size() == 0)
			return 0.0;

		if (currentKey == 0)
			bufferIndex = (bufferIndex + 1) % delayBuffer.size();
		double output = delayBuffer[bufferIndex];
		if (currentKey == 0)
			delayBuffer[bufferIndex] = 0;
		delayBuffer[bufferIndex] += inputValue + feedbackValue * output;

		return output;
	}
};
