#pragma once

#include "envelope.hpp"

double osc(double hertz, double time, double LFOHertz = 0.0, double LFOAmplitude = 0.0);
double pianoKeyFrequency(int keyId);

struct AudioComponent {
	AudioComponent() : input(nullptr) { }
	virtual ~AudioComponent() {};
	virtual double process() = 0;

	static double time;
	AudioComponent* input;
};

struct Master : public AudioComponent {
	double process()
	{
		if (!input)
		{
			std::cout << "[WARNING] no input plugged in master" << std::endl;
			return 0;
		}

		double value = input->process();
		return value;
	}
};

struct KeyboardFrequency : public AudioComponent {
	static unsigned int keyIndex;

	double process()
	{
		return pianoKeyFrequency(keyIndex);
	}
};

struct ADSR : public AudioComponent {
	static sEnvelopeADSR* envelope;

	double process()
	{
		if (!input)
			return 0.0;

		double value = input->process() * envelope->GetAmplitude(time);
		return value;
	}
};

struct Oscillator : public AudioComponent {
	double process()
	{
		if (!input)
			return 0.0;

		double frequency = input->process();
		double value = osc(frequency, time);
		return value;
	}
};
