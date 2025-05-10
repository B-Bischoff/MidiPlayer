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

	double freqToAngularVelocity(double hertz)
	{
		return hertz * 2.0 * M_PI;
	}

	double osc(double hertz, double phase, double time, OscType type, double LFOHertz, double LFOAmplitude)
	{
		double t = freqToAngularVelocity(hertz) * time + LFOAmplitude * hertz * (sin(freqToAngularVelocity(LFOHertz) * time)) + phase;

		switch(type)
		{
			case Sine: return sin(t);
			case Square: return sin(t) > 0 ? 1.0 : -1.0;
			case Triangle: return asin(sin(t)) * (2.0 / M_PI);
			case Saw_Ana: return 0; // [TODO] implement
			case Saw_Dig: return (2.0 / M_PI) * (hertz * M_PI * fmod(time, 1.0 / hertz) - (M_PI / 2.0));
			case Noise: return 2.0 * ((double)rand() / std::numeric_limits<int>::max()) - 1.0;
			default: return 0;
		}
	}
};
