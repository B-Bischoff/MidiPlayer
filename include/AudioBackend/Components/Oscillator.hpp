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
	std::unordered_map<int, double> currentPhases;

	double process(PipelineInfo& info) override
	{
		if (inputs[frequency].size() <= 0)
			return 0.0;

		double deltaTime = previousTime - previouspreviousTime;

		// Only update time and phases once on the first pipeline note
		if ((info.currentKey == info.keyPressed.size() - 1 || info.keyPressed.size() == 0) &&
			info.pipelineInitiator == MASTER_COMPONENT_ID)
		{
			previouspreviousTime = previousTime;
			previousTime = time;
		}

		double output = 0.0;

		double frequencyValue = getInputsValue(frequency, info);
		double phaseValue = getInputsValue(phase, info);
		double phaseIncrement = (2.0 * M_PI * frequencyValue) * deltaTime;

		if (frequencyValue != 300 && type == OscType::Sine)
			std::cout << frequencyValue << std::endl;

		const int phaseIndex = info.keyPressed.size() != 0 ? info.keyPressed[info.currentKey].keyIndex : 0;

		// Init new phases
		if (currentPhases.find(phaseIndex) == currentPhases.end())
			currentPhases[phaseIndex] = 0;
		currentPhases[phaseIndex] += phaseIncrement;

		// Wrap phase between 0 and 2π
		if (currentPhases[phaseIndex] >= 2.0 * M_PI)
			currentPhases[phaseIndex] -= 2.0 * M_PI;

		output += osc(currentPhases[phaseIndex] + phaseValue, frequencyValue);

		return output;
	}

	double osc(double value, double hertz)
	{
		switch(type)
		{
			case Sine: return sin(value);
			case Square: return sin(value) > 0 ? 1.0 : -1.0;
			case Triangle: return asin(sin(value)) * (2.0 / M_PI);
			case Saw_Dig: return (2.0 / M_PI) * (hertz * M_PI * fmod(time, 1.0 / hertz) - (M_PI / 2.0));
			case Noise: return 2.0 * (static_cast<double>(std::rand()) / RAND_MAX) - 1.0;
			default: return 0;
		}
	}
};
