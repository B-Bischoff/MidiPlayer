#pragma once

#include <tsf.h>
#include <set>
#include "path.hpp"
#include "AudioComponent.hpp"
#include "audio_backend.hpp"

struct SoundFontPlayer : public AudioComponent {
	std::set<int> notesOn;
	tsf* tinySoundFont = nullptr;
	double previousTime = -1.0;

	float values[2] = {};

	SoundFontPlayer() : AudioComponent()
	{
		inputs.resize(0); componentName = "SoundFontPlayer";
	}

	double process(const AudioInfos& audioInfos, std::vector<MidiInfo>& keyPressed, int currentKey = 0) override
	{
		if (currentKey != 0 || tinySoundFont == nullptr)
			return 0;

		addNotes(keyPressed);
		removeNotes(keyPressed);

		// Render once per sample (on the first channel), then return the appropriate channel value
		if (previousTime != time)
		{
			tsf_render_float(tinySoundFont, values, 1, 0);
			previousTime = time;
		}
		return static_cast<double>(values[audioInfos.currentChannel]);
	}

	void addNotes(std::vector<MidiInfo>& keyPressed)
	{
		for (const MidiInfo& key : keyPressed)
		{
			if (notesOn.find(key.keyIndex) == notesOn.end())
			{
				notesOn.insert(key.keyIndex);
				tsf_note_on(tinySoundFont, 0, key.keyIndex, (double)key.velocity / 255.0);
			}
		}
	}

	void removeNotes(std::vector<MidiInfo>& keyPressed)
	{
		auto it = notesOn.begin();
		while (it != notesOn.end())
		{
			bool noteStillPlayed = false;
			for (const MidiInfo& key : keyPressed)
			{
				if (key.keyIndex == *it)
				{
					noteStillPlayed = true;
					break;
				}
			}

			if (!noteStillPlayed)
			{
				tsf_note_off(tinySoundFont, 0, *it);
				it = notesOn.erase(it);
			}
			else
				it++;
		}
	}
};
