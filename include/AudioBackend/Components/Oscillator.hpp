#pragma once

#include <cmath>
#include <cstdlib>
#include "AudioComponent.hpp"
#include "audio_backend.hpp"

enum OscType { Sine, Square, Triangle, Saw_Dig, Noise };

struct Oscillator : public AudioComponent {
	enum Inputs { frequency, phase };
	OscType type;

	Oscillator() : AudioComponent() { inputs.resize(2); componentName = "Oscillator"; }

	double previousTime = time;
	double previouspreviousTime = previousTime;
	double currentPhase = 0;

	double process(PipelineInfo& info) override
	{
		if (inputs[frequency].size() <= 0)
			return 0.0;

		double frequencyValue = getInputsValue(frequency, info);
		double phaseValue = getInputsValue(phase, info);

		double phaseIncrement = (2.0 * M_PI * frequencyValue) * (previousTime - previouspreviousTime);

		std::cout << frequencyValue << " " << (time - previousTime) << std::endl;
		if ((info.currentKey == info.keyPressed.size() - 1 || info.keyPressed.size() == 0) && info.pipelineInitiator == MASTER_COMPONENT_ID)
		{
			currentPhase += phaseIncrement;
			previouspreviousTime = previousTime;
			previousTime = time;
		}
		std::cout << ">> " << info.currentKey << " " << info.keyPressed.size() << " " << info.pipelineInitiator << std::endl;

		// Wrap phase between 0 and 2π
		if (currentPhase >= 2.0 * M_PI) currentPhase -= 2.0 * M_PI;

		return sin(currentPhase);

	//	double value = osc(frequencyValue, M_PI * phaseValue, fmod(time, M_PI * 2.0) - M_PI, type);

		//return value;
	}

	double freqToAngularVelocity(double hertz)
	{
		return hertz * 2.0 * M_PI;
	}

	double osc(double hertz, double phase, double time, OscType type)
	{
		//std::cout << time << std::endl;
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
