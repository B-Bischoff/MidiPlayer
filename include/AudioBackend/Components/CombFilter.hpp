#pragma once

#include <algorithm>
#include "AudioComponent.hpp"

struct CombFilter : public AudioComponent {
	enum Input { input, delaySamples, feedback };

	std::vector<double> delayBuffer;
	int bufferIndex = 0;
	double previousTime = -1.0;
	double lastOutput = 0.0;

	CombFilter() : AudioComponent() { inputs.resize(3); componentName = "CombFilter"; }

	std::shared_ptr<AudioComponent> clone() const override {
		return std::make_shared<CombFilter>();
	}

	double process(const AudioInfos& audioInfos) override
	{
		const int delaySamplesValue = static_cast<int>(getInputsValue(delaySamples, audioInfos));
		const double feedbackValue = std::clamp(getInputsValue(feedback, audioInfos), 0.0, 1.0);
		const double inputValue = getInputsValue(input, audioInfos);

		// Resize buffer on delaySamplesValue change
		if (delaySamplesValue > 0 && delaySamplesValue != delayBuffer.size())
		{
			delayBuffer.resize(delaySamplesValue);
			if (bufferIndex >= delayBuffer.size())
				bufferIndex = 0;
		}

		if (delayBuffer.empty())
			return 0.0;

		// process() is called once per output channel (e.g. twice for stereo)
		// with the same `time` value. The delay line must only advance/mutate
		// once per real audio sample, or it effectively runs at double speed
		// and desyncs between channels — so mirror SoundFontPlayer's approach
		// and gate the stateful update on `time` actually changing.
		if (previousTime == time)
			return lastOutput;
		previousTime = time;

		bufferIndex = (bufferIndex + 1) % delayBuffer.size();

		double output = inputValue + feedbackValue * delayBuffer[bufferIndex];
		delayBuffer[bufferIndex] = output;

		lastOutput = output;
		return output;
	}
};
