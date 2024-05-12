#pragma once

#include "AudioComponent.hpp"
#include "audio_backend.hpp"

struct Oscillator : public AudioComponent {
	OscType type;
	std::vector<AudioComponent*> freq;
	std::vector<AudioComponent*> phase;
	std::vector<AudioComponent*> LFO_Hz;
	std::vector<AudioComponent*> LFO_Amplitude;

	Components getInputs() override
	{
		return combineVectorsToForwardList(freq, phase, LFO_Hz, LFO_Amplitude);
	}

	void clearInputs() override
	{
		clearVectors(freq, phase, LFO_Hz, LFO_Amplitude);
	}

	void addInput(const std::string& inputName, AudioComponent* input) override
	{
		// [TODO] remove those "> "
		if (inputName == "> freq") freq.push_back(input);
		else if (inputName == "> phase") phase.push_back(input);
		else if (inputName == "> LFO Hz") LFO_Hz.push_back(input);
		else if (inputName == "> LFO Amplitude") LFO_Amplitude.push_back(input);
		else assert(0 && "[Oscillator node] unknown input");
	}

	double process(std::vector<MidiInfo>& keyPressed, int currentKey = 0) override
	{
		if (!freq.size())
			return 0.0;
		double frequencyValue = getInputsValue(freq, keyPressed, currentKey);
		double phaseValue = getInputsValue(phase, keyPressed, currentKey);
		double LFO_Hz_Value = getInputsValue(LFO_Hz, keyPressed, currentKey);
		double LFO_Amplitude_Value = getInputsValue(LFO_Amplitude, keyPressed, currentKey);
		double value = osc(frequencyValue, M_PI * phaseValue, time, type, LFO_Hz_Value, LFO_Amplitude_Value);

		return value;
	}
};
