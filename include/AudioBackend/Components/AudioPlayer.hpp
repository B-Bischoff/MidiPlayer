#pragma once

#include "AudioComponent.hpp"
#include "AudioFileManager.hpp"

struct AudioPlayer : public AudioComponent {
	::id fileId = 0;
	unsigned int readCursor = 0;
	// Add media control such as pause/resume

	AudioPlayer() : AudioComponent() { inputs.resize(0); componentName = "AudioPlayer"; }

	double process(std::vector<MidiInfo>& keyPressed, int currentKey = 0) override
	{
		// [TODO] handle mono/stereo
		const AudioFile& audioFile = AudioFileManager::getAudioFile(fileId);

		const float value = audioFile.data[readCursor++];
		return value / (float)std::numeric_limits<short>::max();
	}
};
