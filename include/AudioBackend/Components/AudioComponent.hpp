#pragma once

#include "inc.hpp"

struct AudioComponent {
	AudioComponent() { }
	virtual ~AudioComponent() {};

	virtual double process(std::vector<MidiInfo>& keyPressed, int currentKey = 0) = 0;
	virtual Components getInputs() { return Components(); }
	virtual void clearInputs() { }
	virtual void addInput(const std::string& inputName, AudioComponent* input) {
		assert(0 && "Node does not have input");
	}
	virtual double getInputsValue(std::vector<AudioComponent*>& inputs, std::vector<MidiInfo>& keyPressed, int currentKey = 0) {
		double value = 0.0;
		for (AudioComponent* input : inputs)
			value += input->process(keyPressed, currentKey);
		return value;
	}

	static double time;
};

