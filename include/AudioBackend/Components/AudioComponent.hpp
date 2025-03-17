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

	static double time;

	virtual double process(std::vector<MidiInfo>& keyPressed, int currentKey = 0) = 0;

	Components getInputs() const
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

	bool removeInput(AudioComponent* input)
	{
		bool deleted = false;
		for (int i = 0; i < inputs.size(); i++)
		{
			ComponentInput& componentInput = inputs[i];
			auto it = componentInput.begin();
			while (it != componentInput.end())
			{
				if (*it == input)
				{
					deleted = true;
					componentInput.erase(it);
					it = componentInput.begin();
				}
				else
					it++;
			}
		}
		return deleted;
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

	bool idIsDirectChild(const unsigned int id) const
	{
		Components inputs = getInputs();
		for (const AudioComponent* audioComponent : inputs)
		{
			if (audioComponent->id == id)
				return true;
		}
		return false;
	}

	void removeComponentFromBranch(AudioComponent* componentToRemove, const bool deleteComponent, unsigned int depth = 0)
	{
		Components componentInputs = getInputs();
		for (AudioComponent* input : componentInputs)
			input->removeComponentFromBranch(componentToRemove, deleteComponent, depth + 1);

		removeInput(componentToRemove);

		if (deleteComponent && depth == 0)
		{
			Logger::log("Remove Component from Backend") << "REMOVING " << componentToRemove->id << std::endl;
			delete componentToRemove;
		}
	}

	AudioComponent* getAudioComponent(const unsigned int id)
	{
		if (this->id == id)
			return this;

		for (const ComponentInput& componentInputs : inputs) // loop over component inputs
		{
			for (AudioComponent* input : componentInputs) // loop over all the component plugged to one input
			{
				AudioComponent* foundAudioComponent =  input->getAudioComponent(id);
				if (foundAudioComponent != nullptr)
					return foundAudioComponent;
			}
		}
		return nullptr;
	}

	bool idExists(const unsigned int id)
	{
		return getAudioComponent(id) != nullptr;
	}
};
