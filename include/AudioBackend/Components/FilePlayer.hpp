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

		if (fileId == 0)
			return 0;

		const AudioFile& audioFile = AudioFileManager::getAudioFile(fileId);

		const float value = audioFile.data[readCursor];
		readCursor++;
		readCursor++;

		if (readCursor >= audioFile.dataLength - 1)
			readCursor = 0;

		return value / (float)std::numeric_limits<short>::max();
	}
};
