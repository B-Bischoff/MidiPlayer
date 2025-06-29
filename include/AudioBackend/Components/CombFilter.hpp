#pragma once

#include "AudioComponent.hpp"
#include "audio_backend.hpp"

struct CombFilter : public AudioComponent {
	enum Input { input, delaySamples, feedback };

	std::vector<double> delayBuffer;
	int bufferIndex = 0;

	CombFilter() : AudioComponent() { inputs.resize(3); componentName = "CombFilter"; }

	double process(const AudioInfos& audioInfos, std::vector<MidiInfo>& keyPressed, int currentKey = 0) override
	{
		int delaySamplesValue = static_cast<int>(getInputsValue(delaySamples, audioInfos, keyPressed, currentKey));
		double feedbackValue = getInputsValue(feedback, audioInfos, keyPressed, currentKey);
		if (feedbackValue > 1.0) feedbackValue = 1.0;
		double inputValue = getInputsValue(input, audioInfos, keyPressed, currentKey);

		// Resize buffer on delaySamplesValue change
		if (delaySamplesValue > 0 && delaySamplesValue != delayBuffer.size())
		{
			delayBuffer.resize(delaySamplesValue);
			if (bufferIndex >= delayBuffer.size())
				bufferIndex = 0;
		}

		if (delayBuffer.empty())
			return 0.0;

		if (currentKey == 0) // only increment bufferIndex on the first note (playing multiple notes must not increase index)
			bufferIndex = (bufferIndex + 1) % delayBuffer.size();

		double output = inputValue;
		if (currentKey == 0)
		{
			output += feedbackValue * delayBuffer[bufferIndex]; // Add stored sound to the first note
			delayBuffer[bufferIndex] = output; // Replace stored sound with current sound
		}
		else
			delayBuffer[bufferIndex] += inputValue; // Add other notes sound to the buffer

		return output;
	}
};
