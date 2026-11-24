#pragma once

#include <unordered_map>
#include <list>
#include <memory>
#include <algorithm>
#include <limits>
#include "inc.hpp"
#include "Logger.hpp"

struct AudioComponent {
	// A connection to a specific output slot of an upstream AudioComponent.
	// outputIndex selects which of that component's cached outputValues to read
	// most components only ever have a single output (index 0).
	struct Connection {
		std::shared_ptr<AudioComponent> component;
		int outputIndex = 0;
	};

	AudioComponent() : id(nextId++) { outputValues.resize(1, 0.0); }
	virtual ~AudioComponent() {};

	std::vector<std::vector<Connection>> inputs; // Each input can have multiple components connected to it
	std::string componentName; // Used to debug/log things

	static unsigned int nextId;
	unsigned int id;

	static double time;

	// Cached results of the last computeOutputs() call, one entry per output slot.
	// Components with a single output only ever use index 0.
	std::vector<double> outputValues;

	// Compute every output slot of this component for the current sample and
	// store the results into outputValues. Implementations must fill all of
	// outputValues (resized in the constructor to the component's output count).
	virtual void computeOutputs(const AudioInfos& audioInfos) = 0;

	// Create a shallow clone (same type, same parameters, fresh state, no inputs wired)
	virtual std::shared_ptr<AudioComponent> clone() const = 0;

	// Deep clone: clone this node and recursively clone all its inputs.
	std::shared_ptr<AudioComponent> deepClone() const;

	bool isMidiSource() const;

	// Components with their own release stage (ADSR, SoundFontPlayer) override
	// these so Polyphony knows when a released voice can be freed.
	virtual bool hasReleaseTail() const { return false; }
	virtual bool isReleaseFinished() const { return true; }

	void clearInputs()
	{
		for (auto& input : inputs)
			input.clear();
	}

	void addInput(const unsigned int& index, std::shared_ptr<AudioComponent> newInput, int outputIndex = 0)
	{
		if (inputs.size() <= index)
		{
			Logger::log("AudioComponent", Error) << "Out of bound index in addInput method. Index: " << index << " Input size: " << inputs.size() << std::endl;
			exit(1);
		}

		inputs[index].push_back({ newInput, outputIndex });
	}

	bool removeInput(const std::shared_ptr<AudioComponent>& input)
	{
		for (auto& inputList : inputs)
		{
			auto it = std::find_if(inputList.begin(), inputList.end(), [&](const Connection& conn) { return conn.component == input; });
			if (it != inputList.end())
			{
				inputList.erase(it);
				return true;
			}
		}
		return false;
	}

	// Ensure computeOutputs() has run for this sample (gated by static time, so
	// it only runs once even if called multiple times per sample, e.g. once per
	// stereo channel) and return the requested output slot.
	double getOutput(const AudioInfos& audioInfos, int outputIndex = 0)
	{
		if (_lastProcessedTime != time)
		{
			computeOutputs(audioInfos);
			_lastProcessedTime = time;
		}

		if (outputIndex < 0 || (size_t)outputIndex >= outputValues.size())
		{
			Logger::log("AudioComponent", Error) << "Out of bound output index requested: " << outputIndex << std::endl;
			exit(1);
		}

		return outputValues[outputIndex];
	}

	virtual double getInputsValue(const unsigned int& index, const AudioInfos& audioInfos)
	{
		if (inputs.size() <= index)
		{
			Logger::log("AudioComponent", Error) << "Out of bound index in getInputValue method" << std::endl;
			exit(1);
		}

		std::vector<Connection>& input = inputs[index];

		double value = 0.0;
		for (auto& conn : input)
			value += conn.component->getOutput(audioInfos, conn.outputIndex);
		return value;
	}

private:
	// Sentinel NaN: never equal to any real time value (including itself),
	// so the very first getOutput() call always triggers computeOutputs().
	double _lastProcessedTime = std::numeric_limits<double>::quiet_NaN();
};
