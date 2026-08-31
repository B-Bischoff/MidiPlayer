#pragma once

#include <vector>
#include "AudioComponent.hpp"

struct MidiSourceComponent : public AudioComponent {
	MidiSourceComponent() : AudioComponent() {}
	virtual ~MidiSourceComponent() {}

	virtual std::vector<MidiInfo> processMidi(const AudioInfos& audioInfos) = 0;

	// [TODO] COMMENTED FOR BACKWARD COMPATIBILITY (WILL BE REMOVED IN THE FUTURE)
	// MidiSourceComponent does not produce audio, so we override the process function to return 0.0
	//double process(const AudioInfos& audioInfos, std::vector<MidiInfo>& keyPressed, int currentKey = 0) override
	//{
	//	return 0.0;
	//}
};
