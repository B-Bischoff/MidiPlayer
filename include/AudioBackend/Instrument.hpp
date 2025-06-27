#pragma once

#include "AudioBackend/Components/Master.hpp"

class Instrument {
public:
	Master master;
	std::string name;
	float volume = 1.0f;

	double process(const AudioInfos& audioInfos, std::vector<MidiInfo>& keyPressed);
};
