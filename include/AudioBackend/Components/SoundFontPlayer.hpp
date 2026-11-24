#pragma once

#include <tsf.h>
#include "path.hpp"
#include "AudioComponent.hpp"
#include "AudioBackend/VoiceContext.hpp"

// Plays notes through a loaded SoundFont (.sf2) instrument. Works both
// standalone (driven directly by a note-number input, e.g. a Number node)
// and inside a Polyphony-cloned voice (driven by activeVoiceContext) through
// a single unified trigger path — see process() below.
//
// Each SoundFontPlayer owns a private tsf instance created with tsf_copy():
// the font data (presets, samples) is shared and ref-counted, but the
// voice/render state is per instance. This is required because every
// Polyphony voice gets its own SoundFontPlayer clone, and rendering a shared
// tsf from each clone would advance the synth once per voice per sample.
struct SoundFontPlayer : public AudioComponent {
private:
	tsf* tinySoundFont = nullptr; // Owned copy, released in the destructor

public:
	enum Inputs { midiInput, velocityInput };

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
		inputs.resize(2); componentName = "SoundFontPlayer";
	}

	~SoundFontPlayer() override { tsf_close(tinySoundFont); }

	// Owns a raw tsf pointer: copying would double-free it
	SoundFontPlayer(const SoundFontPlayer&) = delete;
	SoundFontPlayer& operator=(const SoundFontPlayer&) = delete;

	std::shared_ptr<AudioComponent> clone() const override {
		auto c = std::make_shared<SoundFontPlayer>();
		c->setSoundFont(tinySoundFont); // Shares font data, fresh voice state
		return c;
	}

	// Replace the instrument with a private copy of `source` (nullptr unloads it).
	// The caller keeps ownership of `source` and may close it afterwards.
	void setSoundFont(tsf* source)
	{
		tsf_close(tinySoundFont);
		tinySoundFont = source ? tsf_copy(source) : nullptr;
		noteOn = 0;
		noteOffSent = false;
	}

	bool hasSoundFont() const { return tinySoundFont != nullptr; }

	bool hasReleaseTail() const override { return true; }
	// tsf keeps counting a voice as active until its release stage has ended
	bool isReleaseFinished() const override { return !tinySoundFont || tsf_active_voice_count(tinySoundFont) == 0; }

	void computeOutputs(const AudioInfos& audioInfos) override
	{
		if (tinySoundFont == nullptr)
		{
			outputValues[0] = 0.0;
			return;
		}

		double midiValue = getInputsValue(midiInput, audioInfos);
		double velocityValue = getInputsValue(velocityInput, audioInfos);

		// The note input can be any signal (e.g. a note scaled by a Multiplier):
		// snap it to the nearest key and treat anything outside the MIDI key
		// range as "no note" (0), since tsf can't play it anyway.
		int note = static_cast<int>(std::lround(midiValue));
		if (note < 1 || note > 127)
			note = 0;

		bool noteHeld;
		unsigned int gen;

		if (activeVoiceContext)
		{
			// Inside a Polyphony voice: the note value stays constant for the
			// whole lifetime of the voice (release tail included), so we
			// can't edge-detect note-off from the input value alone. Use the
			// voice's own generation/releasing flags instead.
			noteHeld = note != 0 && !activeVoiceContext->releasing;
			gen = activeVoiceContext->generation;
		}
		else
		{
			// Standalone usage: derive an equivalent generation/held pair
			// directly from the input signal.
			bool held = note != 0;
			bool changed = held && standaloneLastNote != note;
			if ((held && !standaloneHeld) || changed)
				standaloneGeneration++;
			standaloneHeld = held;
			if (held) standaloneLastNote = note;
			noteHeld = held;
			gen = standaloneGeneration;
		}

		// Single unified trigger path, regardless of polyphonic context.
		if (gen != lastSeenGeneration && noteHeld)
		{
			lastSeenGeneration = gen;
			if (noteOn != 0)
				tsf_note_off(tinySoundFont, 0, noteOn); // Stop previous note before starting the new one
			tsf_note_on(tinySoundFont, 0, note, (double)velocityValue / 255.0f);
			noteOn = note;
			noteOffSent = false;
		}
		else if (!noteHeld && !noteOffSent && noteOn != 0)
		{
			tsf_note_off(tinySoundFont, 0, noteOn);
			noteOffSent = true;
		}

		// Render audio once per sample (computeOutputs() is only invoked once
		// per real sample by the base class, even if process()/getOutput() is
		// called multiple times, e.g. once per stereo channel).
		tsf_render_float(tinySoundFont, values, 1, 0);
		outputValues[0] = static_cast<double>(values[0]);
	}
};

