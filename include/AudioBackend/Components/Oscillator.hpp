#pragma once

#include "AudioComponent.hpp"
#include "audio_backend.hpp"

struct Oscillator : public AudioComponent {
	enum Inputs { frequency, phase, LFO_Hz, LFO_Amplitude };
	OscType type;

	Oscillator() : AudioComponent() { inputs.resize(4); componentName = "Oscillator"; }

	double process(std::vector<MidiInfo>& keyPressed, int currentKey = 0) override
	{
		if (inputs[frequency].size() <= 0)
			return 0.0;

		double frequencyValue = getInputsValue(frequency, keyPressed, currentKey);
		double phaseValue = getInputsValue(phase, keyPressed, currentKey);
		double LFO_Hz_Value = getInputsValue(LFO_Hz, keyPressed, currentKey);
		double LFO_Amplitude_Value = getInputsValue(LFO_Amplitude, keyPressed, currentKey);

		double value = osc(frequencyValue, M_PI * phaseValue, time, type, LFO_Hz_Value, LFO_Amplitude_Value);

		return value;
	}
};
