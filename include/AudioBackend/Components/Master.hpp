#pragma once

#include "AudioComponent.hpp"
#include "audio_backend.hpp"

struct Master : public AudioComponent {
private:
	bool showWarning = true;

public:
	enum Inputs { input };

	Master() : AudioComponent() { inputs.resize(1); componentName = "Master"; }

	virtual ~Master()
	{
		Components inputs = getInputs();
		for (auto& input : inputs)
			deleteComponentAndInputs(input);
	}

	double process(std::vector<MidiInfo>& keyPressed, int currentKey = 0) override
	{
		if (!inputs.size())
		{
			if (showWarning)
			{
				showWarning = false;
				Logger::log("Audio", Warning) << "No input plugged to master." << std::endl;
			}
			return 0;
		}

		showWarning = true;

		double value = 0.0;


		for (AudioComponent* input : inputs[input])
		{
			int i = 0;
			do
			{
				value += input->process(keyPressed, i);
			} while (++i < keyPressed.size());
		}

		return value;
	}

	void deleteComponentAndInputs(AudioComponent* component)
	{
		Components inputs = component->getInputs();

		auto it = inputs.begin();
		while (it != inputs.end())
		{
			if (idExists((*it)->id))
				deleteComponentAndInputs(*it);
			else
				it++;
			inputs = component->getInputs();
			it = inputs.begin();
			if (it == inputs.end())
				break;
		}

		removeComponentFromBranch(component, true);
	}
};
