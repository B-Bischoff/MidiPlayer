#pragma once

#include <tsf.h>
#include "path.hpp"
#include "AudioComponent.hpp"
#include "AudioBackend/VoiceContext.hpp"

// Plays notes through a loaded SoundFont (.sf2) instrument. Works both
// standalone (driven directly by a note-number input, e.g. a Number node)
// and inside a Polyphony-cloned voice (driven by activeVoiceContext) through
// a single unified trigger path — see process() below.
struct SoundFontPlayer : public AudioComponent {
	enum Inputs { midiInput };

	tsf* tinySoundFont = nullptr;
	double previousTime = 0.0;
	float values[2] = {};

	int noteOn = 0; // Currently sounding note (0 = none)
	unsigned int lastSeenGeneration = 0;
	bool noteOffSent = false;

	// Standalone fallback state (no Polyphony/MIDI source upstream): derive a
	// synthetic generation from the input signal itself instead of relying on
	// activeVoiceContext. A rising edge from silence, or a direct note-number
	// change while held, both count as a new "voice assignment".
	bool standaloneHeld = false;
	int standaloneLastNote = 0;
	unsigned int standaloneGeneration = 0;

	SoundFontPlayer() : AudioComponent()
	{
		inputs.resize(1); componentName = "SoundFontPlayer";
	}

	std::shared_ptr<AudioComponent> clone() const override {
		auto c = std::make_shared<SoundFontPlayer>();
		c->tinySoundFont = tinySoundFont; // Share the loaded soundfont
		return c;
	}

	double process(const AudioInfos& audioInfos) override
	{
		if (tinySoundFont == nullptr)
			return 0.0;

		double midiValue = getInputsValue(midiInput, audioInfos);

		bool noteHeld;
		unsigned int gen;

		if (activeVoiceContext)
		{
			// Inside a Polyphony voice: the note value stays constant for the
			// whole lifetime of the voice (release tail included), so we
			// can't edge-detect note-off from the input value alone. Use the
			// voice's own generation/releasing flags instead.
			noteHeld = midiValue != 0.0 && !activeVoiceContext->releasing;
			gen = activeVoiceContext->generation;
		}
		else
		{
			// Standalone usage: derive an equivalent generation/held pair
			// directly from the input signal.
			bool held = midiValue != 0.0;
			bool changed = held && standaloneLastNote != (int)midiValue;
			if ((held && !standaloneHeld) || changed)
				standaloneGeneration++;
			standaloneHeld = held;
			if (held) standaloneLastNote = (int)midiValue;
			noteHeld = held;
			gen = standaloneGeneration;
		}

		// Single unified trigger path, regardless of polyphonic context.
		if (gen != lastSeenGeneration && noteHeld)
		{
			lastSeenGeneration = gen;
			if (noteOn != 0)
				tsf_note_off(tinySoundFont, 0, noteOn); // Stop previous note before starting the new one
			tsf_note_on(tinySoundFont, 0, (int)midiValue, 127.0f / 255.0f);
			noteOn = (int)midiValue;
			noteOffSent = false;
		}
		else if (!noteHeld && !noteOffSent && noteOn != 0)
		{
			tsf_note_off(tinySoundFont, 0, noteOn);
			noteOffSent = true;
		}

		// Render audio only once per sample, even if process() is called multiple times for the same time step (e.g stereo channels)
		if (previousTime != time)
		{
			tsf_render_float(tinySoundFont, values, 1, 0);
			previousTime = time;
			return static_cast<double>(values[0]);
		}
		return static_cast<double>(values[1]);
	}
};

