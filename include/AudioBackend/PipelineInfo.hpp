#pragma once

#include <vector>
#include "inc.hpp"

struct PipelineInfo {
	enum CurrentEar { Left, Right, Mono };

	std::vector<MidiInfo>& keyPressed;
	CurrentEar currentEar = Mono;
	unsigned int currentKey = 0;
	unsigned int pipelineInitiator = 0;

	PipelineInfo(std::vector<MidiInfo>& keyPressed, CurrentEar currentEar = Mono, unsigned int currentKey = 0, unsigned int pipelineInitiator = 0)
		: keyPressed(keyPressed), currentEar(currentEar), currentKey(currentKey), pipelineInitiator(pipelineInitiator) { }
};
