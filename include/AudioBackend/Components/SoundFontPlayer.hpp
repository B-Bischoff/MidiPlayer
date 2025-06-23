#pragma once

#include <tsf.h>
#include <fluidsynth.h>
#include <set>
#include "path.hpp"
#include "AudioComponent.hpp"
#include "audio_backend.hpp"

struct SoundFontPlayer : public AudioComponent {
	fluid_synth_t* synth = nullptr;
	fluid_settings_t* settings = nullptr;

	std::set<int> notesOn;
	tsf* tinySoundFont = nullptr;

	SoundFontPlayer() : AudioComponent()
	{
		inputs.resize(0); componentName = "SoundFontPlayer";
	}

	~SoundFontPlayer()
	{
		if (synth) delete_fluid_synth(synth);
		if (settings) delete_fluid_settings(settings);
	}

	double process(std::vector<MidiInfo>& keyPressed, int currentKey = 0) override
	{
		if (currentKey != 0 || tinySoundFont == nullptr)
			return 0;

		addNotes(keyPressed);
		removeNotes(keyPressed);

		float synthValue = 0;
		//if (fluid_synth_write_float(synth, 1, &synthValue, 0, 2, &synthValue, 0, 2) != FLUID_OK)
		//	Logger::log("Fluidynth", Warning) << "write error" << std::endl;
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

	bool loadSoundFontFile(const fs::path& filepath)
	{
		tinySoundFont = tsf_load_filename(filepath.string().c_str());
		tsf_set_output(tinySoundFont, TSF_MONO, 44100, 0); //sample rate

		// Clear previous notes on
		for (const int& note : notesOn)
			fluid_synth_noteoff(synth, 0, note);
		notesOn.clear();

		// Delete previous loaded instrument
		if (synth != nullptr)
			delete_fluid_synth(synth);
		if (settings != nullptr)
			delete_fluid_settings(settings);

		settings = new_fluid_settings();
		synth = new_fluid_synth(settings);
		int sfid = fluid_synth_sfload(synth, filepath.string().c_str(), 1);

		if (sfid == FLUID_FAILED)
		{
			delete_fluid_synth(synth); synth = nullptr;
			delete_fluid_settings(settings); settings = nullptr;
			Logger::log("SoundFont Node", Error) << "Unable to load file: " << fs::path(filepath).filename().string() << std::endl;
			return true;
		}
		if (fluid_synth_program_select(synth, 0, sfid, 0, 0) != FLUID_OK)
		{
			delete_fluid_settings(settings);
			delete_fluid_synth(synth);
			Logger::log("SoundFont Node", Error) << "Synth selection error" << std::endl;
			return true;
		}

		return false;
	}
};
