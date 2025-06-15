#pragma once

#include <fluidsynth.h>
#include <set>
#include "AudioComponent.hpp"
#include "audio_backend.hpp"

struct SoundFontPlayer : public AudioComponent {
	fluid_synth_t* synth = nullptr;
	std::set<int> notesOn;

	SoundFontPlayer() : AudioComponent()
	{
		inputs.resize(0); componentName = "SoundFontPlayer";

		fluid_settings_t* fluidSettings = new_fluid_settings();
		synth = new_fluid_synth(fluidSettings);
		int sfid = fluid_synth_sfload(synth, "/home/bbischoff/Downloads/Yamaha_C7__Normalized_.sf2", 1);
		if (sfid == FLUID_FAILED)
		{
			Logger::log("FLUID", Error) << "load error" << std::endl;
			exit(1);
		}
		if (fluid_synth_program_select(synth, 0, sfid, 0, 0) != FLUID_OK)
		{
			Logger::log("FLUID", Error) << "synth select error" << std::endl;
			exit(1);
		}
	}

	double process(std::vector<MidiInfo>& keyPressed, int currentKey = 0) override
	{
		if (currentKey != 0)
			return 0;

		addNotes(keyPressed);
		removeNotes(keyPressed);

		float synthValue = 0;
		if (fluid_synth_write_float(synth, 1, &synthValue, 0, 2, &synthValue, 0, 2) != FLUID_OK)
			Logger::log("Fluidynth", Warning) << "write error" << std::endl;

		return static_cast<double>(synthValue);
	}

	void addNotes(std::vector<MidiInfo>& keyPressed)
	{
		for (const MidiInfo& key : keyPressed)
		{
			if (notesOn.find(key.keyIndex) == notesOn.end())
			{
				notesOn.insert(key.keyIndex);
				// Midiplayer velocity is ranged between 0 & 255, however fluisynth
				// uses a range between 0 & 127.
				if (fluid_synth_noteon(synth, 0, key.keyIndex, key.velocity / 2) != FLUID_OK)
				{
					Logger::log("FLUID", Error) << "synth noteon error" << std::endl;
					exit(1);
				}
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
				if (fluid_synth_noteoff(synth, 0, *it) != FLUID_OK)
				{
					Logger::log("FLUID", Error) << "synth noteoff error" << std::endl;
					exit(1);
				}
				it = notesOn.erase(it);
			}
			else
				it++;
		}
	}
};
