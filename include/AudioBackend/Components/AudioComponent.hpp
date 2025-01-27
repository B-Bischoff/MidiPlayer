#pragma once

#include "inc.hpp"
#include <unordered_map>
#include "Logger.hpp"
#include <list>

struct AudioComponent {
	AudioComponent() : id(nextId++) { }
	virtual ~AudioComponent() {};

	std::vector<ComponentInput> inputs;
	std::string componentName; // Used to debug/log things

	static unsigned int nextId;
	unsigned int id;

	virtual double process(std::vector<MidiInfo>& keyPressed, int currentKey = 0) = 0;

	Components getInputs()
	{
		Components result;
		for (auto& input : inputs) // Loop over all inputs
		{
			for (AudioComponent* component : input) // Loop over all components plugged on that input
				result.push_front(component);
		}
		return result;
	}

	void clearInputs()
	{
		for (auto& input : inputs)
			input.clear();
	}

	void addInput(const unsigned int& index, AudioComponent* newInput)
	{
		if (inputs.size() <= index)
		{
			Logger::log("AudioComponent", Error) << "Out of bound index in addInput method. Index: " << index << " Input size: " << inputs.size() << std::endl;
			exit(1);
		}

		inputs[index].push_back(newInput);
	}

	virtual double getInputsValue(const unsigned int& index, std::vector<MidiInfo>& keyPressed, int currentKey = 0)
	{
		if (inputs.size() <= index)
		{
			Logger::log("AudioComponent", Error) << "Out of bound index in getInputValue method" << std::endl;
			exit(1);
		}

		ComponentInput& input = inputs[index];

		double value = 0.0;
		for (AudioComponent* component : input)
			value += component->process(keyPressed, currentKey);
		return value;
	}

	static double time;
};
