#pragma once

#include "AudioComponent.hpp"
#include "audio_backend.hpp"

struct Master : public AudioComponent {
private:
	bool showWarning = true;

public:
	std::vector<AudioComponent*> inputs;

	Components getInputs() override
	{
		return combineVectorsToForwardList(inputs);
	}

	void addInput(const std::string& inputName, AudioComponent* input) override
	{
		// [TODO] check inputName even thought it is not necessary for 1 input nodes ?
		inputs.push_back(input);
	}

	void clearInputs() override
	{
		clearVectors(inputs);
	}

	double process(std::vector<MidiInfo>& keyPressed, int currentKey = 0) override
	{
		if (!inputs.size())
		{
			if (showWarning)
			{
				showWarning = false;
				std::cout << "[WARNING] no input plugged in master" << std::endl;
			}
			return 0;
		}

		showWarning = true;

		double value = 0.0;


		for (AudioComponent* input : inputs)
		{
			int i = 0;
			do
			{
				value += input->process(keyPressed, i);
			} while (++i < keyPressed.size());
		}

		return value;
	}
};
