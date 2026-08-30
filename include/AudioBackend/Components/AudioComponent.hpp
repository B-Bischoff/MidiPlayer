#pragma once

#include <unordered_map>
#include "Logger.hpp"
#include <list>
#include <memory>
#include <algorithm>
#include "inc.hpp"

struct AudioComponent {
	AudioComponent() : id(nextId++) { }
	virtual ~AudioComponent() {};

	std::vector<std::vector<std::shared_ptr<AudioComponent>>> inputs; // Each input can have multiple components connected to it
	std::string componentName; // Used to debug/log things

	static unsigned int nextId;
	unsigned int id;

	static double time;

	virtual double process(const AudioInfos& audioInfos, std::vector<MidiInfo>& keyPressed, int currentKey = 0) = 0;

	void clearInputs()
	{
		for (auto& input : inputs)
			input.clear();
	}

	void addInput(const unsigned int& index, std::shared_ptr<AudioComponent> newInput)
	{
		if (inputs.size() <= index)
		{
			Logger::log("AudioComponent", Error) << "Out of bound index in addInput method. Index: " << index << " Input size: " << inputs.size() << std::endl;
			exit(1);
		}

		inputs[index].push_back(newInput);
	}

	bool removeInput(const std::shared_ptr<AudioComponent>& input)
	{
		for (auto& inputList : inputs)
		{
			auto it = std::find(inputList.begin(), inputList.end(), input);
			if (it != inputList.end())
			{
				inputList.erase(it);
				return true;
			}
		}
		return false;
	}

	virtual double getInputsValue(const unsigned int& index, const AudioInfos& audioInfos, std::vector<MidiInfo>& keyPressed, int currentKey = 0)
	{
		if (inputs.size() <= index)
		{
			Logger::log("AudioComponent", Error) << "Out of bound index in getInputValue method" << std::endl;
			exit(1);
		}

		std::vector<std::shared_ptr<AudioComponent>>& input = inputs[index];

		double value = 0.0;
		for (auto& component : input)
			value += component->process(audioInfos, keyPressed, currentKey);
		return value;
	}
};
