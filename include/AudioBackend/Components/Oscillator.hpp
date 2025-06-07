#pragma once

#include <cstdlib>
#include "AudioComponent.hpp"
#include "audio_backend.hpp"

enum OscType { Sine, Square, Triangle, Saw_Dig, Noise };

struct Oscillator : public AudioComponent {
	enum Inputs { frequency, phase };
	OscType type;

	Oscillator() : AudioComponent() { inputs.resize(2); componentName = "Oscillator"; }

	double process(std::vector<MidiInfo>& keyPressed, int currentKey = 0) override
	{
		if (inputs[frequency].size() <= 0)
			return 0.0;

		double frequencyValue = getInputsValue(frequency, keyPressed, currentKey);
		double phaseValue = getInputsValue(phase, keyPressed, currentKey);

		double value = osc(frequencyValue, M_PI * phaseValue, time, type);

		return value;
	}

	double freqToAngularVelocity(double hertz)
	{
		return hertz * 2.0 * M_PI;
	}

	double osc(double hertz, double phase, double time, OscType type)
	{
		double t = freqToAngularVelocity(hertz) * time + phase;

		switch(type)
		{
			case Sine: return sin(t);
			case Square: return sin(t) > 0 ? 1.0 : -1.0;
			case Triangle: return asin(sin(t)) * (2.0 / M_PI);
			case Saw_Dig: return (2.0 / M_PI) * (hertz * M_PI * fmod(time, 1.0 / hertz) - (M_PI / 2.0));
			case Noise: return 2.0 * (static_cast<double>(std::rand()) / RAND_MAX) - 1.0;
			default: return 0;
		}
	}
};
