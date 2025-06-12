#pragma once

#include <tsf.h>
#include <set>
#include "path.hpp"
#include "AudioComponent.hpp"
#include "audio_backend.hpp"

struct SoundFontPlayer : public AudioComponent {
	std::set<int> notesOn;
	tsf* tinySoundFont = nullptr;

	SoundFontPlayer() : AudioComponent()
	{
		inputs.resize(0); componentName = "SoundFontPlayer";
	}

	double process(PipelineInfo& info) override
	{
		if (info.currentKey != 0 || tinySoundFont == nullptr)
			return 0;

		addNotes(info.keyPressed);
		removeNotes(info.keyPressed);

		float synthValue = 0;
		tsf_render_float(tinySoundFont, &synthValue, 1, 0);

		return static_cast<double>(synthValue);
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
