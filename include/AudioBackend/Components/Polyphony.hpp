#pragma once

#include "AudioComponent.hpp"
#include "MidiSourceComponent.hpp"

struct Polyphony : public AudioComponent {
	enum Inputs { audioTemplate };

	MidiSourceComponent* midiSource = nullptr;

	Polyphony() : AudioComponent() {
		inputs.resize(1);
		componentName = "Polyphony";
	}

	double process(const AudioInfos& audioInfos, std::vector<MidiInfo>& keyPressed, int currentKey = 0) override
	{
		// Pass-through for now — polyphonic voice management will be implemented later
		if (inputs[audioTemplate].empty())
			return 0.0;
		return getInputsValue(audioTemplate, audioInfos, keyPressed, currentKey);
	}
};
