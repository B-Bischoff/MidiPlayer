#pragma once

#include "AudioBackend/Components/Master.hpp"
#include "AudioBackend/Components/KeyboardFrequency.hpp"

class Instrument {
public:
	Master master;
	std::string name;
	float volume = 1.0f;

	double process(const AudioInfos& audioInfos, std::vector<MidiInfo>& keyPressed);

	// Set keyPressed reference on all KeyboardFrequency nodes in the graph
	void updateMidiSources(std::vector<MidiInfo>& keyPressed);

private:
	void findKeyboardFrequencies(const std::shared_ptr<AudioComponent>& node,
		std::vector<KeyboardFrequency*>& result,
		std::unordered_set<unsigned int>& visited);
};
