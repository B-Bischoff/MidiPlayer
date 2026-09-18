#pragma once

#include "AudioComponent.hpp"

struct MidiToFreq : public AudioComponent {
	enum Inputs { midiInput };
	MidiToFreq() : AudioComponent() { inputs.resize(1); componentName = "MidiToFreq"; }

	std::shared_ptr<AudioComponent> clone() const override {
		auto c = std::make_shared<MidiToFreq>();
		return c;
	}

	void computeOutputs(const AudioInfos& audioInfos) override
	{
		if (inputs[midiInput].size() <= 0)
		{
			outputValues[0] = 0.0;
			return;
		}

		double midiValue = getInputsValue(midiInput, audioInfos);
		outputValues[0] = pianoKeyFrequency(midiValue);
	}

	double pianoKeyFrequency(int keyId)
	{
		double A4Frequency = 440.0;
		int keysDifference = keyId - 69;
		double semitoneRatio = pow(2.0, 1.0/12.0);
		return A4Frequency * pow(semitoneRatio, keysDifference);
	}
};
