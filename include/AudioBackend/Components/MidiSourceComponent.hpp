#pragma once

#include <vector>
#include "AudioComponent.hpp"

struct MidiSourceComponent : public AudioComponent {
	MidiSourceComponent() : AudioComponent() {}
	virtual ~MidiSourceComponent() {}

	virtual std::vector<MidiEvent> processMidi(const AudioInfos& audioInfos) = 0;

	void computeOutputs(const AudioInfos& audioInfos) override
	{
		// MidiSourceComponents produce MIDI events, not audio
		outputValues[0] = 0.0;
	}
};
