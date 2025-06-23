#pragma once

#include <tsf.h>
#include <fluidsynth.h>
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

	~SoundFontPlayer()
	{
		// Delete previous loaded instrument
		if (tinySoundFont != nullptr)
		{
			tsf_note_off_all(tinySoundFont);
			tsf_close(tinySoundFont);
		}
	}

	double process(std::vector<MidiInfo>& keyPressed, int currentKey = 0) override
	{
		if (currentKey != 0 || tinySoundFont == nullptr)
			return 0;

		addNotes(keyPressed);
		removeNotes(keyPressed);

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

	bool loadSoundFontFile(const fs::path& filepath, const int& sampleRate)
	{
		notesOn.clear();

		// Delete previous loaded instrument
		if (tinySoundFont != nullptr)
		{
			tsf_note_off_all(tinySoundFont);
			tsf_close(tinySoundFont);
		}
		tinySoundFont = nullptr;

		tinySoundFont = tsf_load_filename(filepath.string().c_str());

		if (tinySoundFont == nullptr)
		{
			Logger::log("SoundFontPlayer", Error) << "Could not open file: " << filepath.string() << std::endl;
			return true;
		}

		tsf_set_output(tinySoundFont, TSF_MONO, sampleRate, 0);

		return false;
	}
};
