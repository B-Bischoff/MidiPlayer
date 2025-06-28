#pragma once

#include <cstdlib>
#include "AudioComponent.hpp"
#include "audio_backend.hpp"

enum OscType { Sine, Square, Triangle, Saw_Dig, WhiteNoise, PinkNoise, BrownianNoise };

struct Oscillator : public AudioComponent {
	enum Inputs { frequency, phase };
	OscType type;

	Oscillator() : AudioComponent() { inputs.resize(2); componentName = "Oscillator"; }
	double pink_b0 = 0, pink_b1 = 0, pink_b2 = 0;
	double brownLast = 0.0;

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

	double whiteNoise()
	{
		return 2.0 * (static_cast<double>(std::rand()) / RAND_MAX) - 1.0;
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
			case WhiteNoise: return whiteNoise();
			case PinkNoise: {
				double white = whiteNoise();
				// Paul Kellet’s refined pink noise filter
				pink_b0 = 0.99765 * pink_b0 + white * 0.0990460;
				pink_b1 = 0.96300 * pink_b1 + white * 0.2965164;
				pink_b2 = 0.57000 * pink_b2 + white * 1.0526913;
				// Sound is by default really loud, divide result to prevent it from breaking my ears
				return (pink_b0 + pink_b1 + pink_b2 + white * 0.1848) / 20.0;
			}
			case BrownianNoise: {
				double white = whiteNoise();
				// Integrate (bounded to prevent drift)
				brownLast += white * 0.02;
				brownLast = std::clamp(brownLast, -1.0, 1.0);
				return brownLast;
			}

			default: return 0;
		}
	}
};
