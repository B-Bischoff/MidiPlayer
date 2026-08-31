#pragma once

#include <unordered_set>
#include "AudioComponent.hpp"

struct Master : public AudioComponent {
private:
	bool showWarning = true;

public:
	enum Inputs { input };

	Master() : AudioComponent() { inputs.resize(1); componentName = "Master"; }

	std::shared_ptr<AudioComponent> clone() const override {
		return std::make_shared<Master>();
	}

	double process(const AudioInfos& audioInfos, std::vector<MidiInfo>& keyPressed, int currentKey = 0) override
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


		for (auto& input : inputs[input])
		{
			int i = 0;
			do
			{
				value += input->process(audioInfos, keyPressed, i);
			} while (++i < keyPressed.size());
		}

		return value;
	}
};
