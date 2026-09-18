#pragma once

#include <algorithm>
#include "AudioComponent.hpp"

struct CombFilter : public AudioComponent {
	enum Input { input, delaySamples, feedback };

	std::vector<double> delayBuffer;
	int bufferIndex = 0;

	CombFilter() : AudioComponent() { inputs.resize(3); componentName = "CombFilter"; }

	std::shared_ptr<AudioComponent> clone() const override {
		return std::make_shared<CombFilter>();
	}

	void computeOutputs(const AudioInfos& audioInfos) override
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
		{
			outputValues[0] = 0.0;
			return;
		}

		// computeOutputs() is only invoked once per real sample (gated by the
		// base class on `time`), so the delay line can safely advance here
		// even though process()/getOutput() may be called multiple times per
		// sample (e.g. once per stereo channel).
		bufferIndex = (bufferIndex + 1) % delayBuffer.size();

		double output = inputValue + feedbackValue * delayBuffer[bufferIndex];
		delayBuffer[bufferIndex] = output;

		outputValues[0] = output;
	}
};
