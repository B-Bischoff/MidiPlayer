#pragma once

#include <vector>
#include "AudioComponent.hpp"

struct MidiSourceComponent : public AudioComponent {
	MidiSourceComponent() : AudioComponent() {}
	virtual ~MidiSourceComponent() {}

	virtual std::vector<MidiEvent> processMidi(const AudioInfos& audioInfos) = 0;

	double process(const AudioInfos& audioInfos) override
	{
		// MidiSourceComponents produce MIDI events, not audio
		return 0.0;
	}
};
