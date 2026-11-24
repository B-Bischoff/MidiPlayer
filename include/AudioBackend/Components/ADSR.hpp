#pragma once

#include "AudioComponent.hpp"
#include "AudioBackend/VoiceContext.hpp"
#include "envelope.hpp"

struct ADSR : public AudioComponent {
public:
	enum Inputs { input, trigger };

	sEnvelopeADSR reference; // Used to store envelope settings value

	// One envelope per clone (each polyphonic voice gets its own ADSR clone)
	sEnvelopeADSR voiceEnvelope;
	unsigned int lastSeenGeneration = 0;

	// Fallback state used when this ADSR isn't inside a Polyphony-cloned
	// voice (no MIDI source upstream, e.g. driven directly by a trigger
	// signal). Derives a synthetic "note held" gate + generation from the
	// trigger input's sign instead of relying on activeVoiceContext.
	bool standaloneHeld = false;
	unsigned int standaloneGeneration = 0;

	ADSR() : AudioComponent() { inputs.resize(2); componentName = "ADSR"; }

	std::shared_ptr<AudioComponent> clone() const override {
		auto c = std::make_shared<ADSR>();
		c->reference = reference;
		return c;
	}

	bool hasReleaseTail() const override { return true; }
	bool isReleaseFinished() const override { return voiceEnvelope.phase == Phase::Inactive; }

	void computeOutputs(const AudioInfos& audioInfos) override
	{
		if (!inputs.size())
		{
			outputValues[0] = 0.0;
			return;
		}

		double inputValue = getInputsValue(input, audioInfos);
		double triggerValue = getInputsValue(trigger, audioInfos);

		bool noteHeld;
		unsigned int gen;

		if (activeVoiceContext)
		{
			noteHeld = activeVoiceContext->noteInfo.keyIndex != 0 && !activeVoiceContext->releasing;
			gen = activeVoiceContext->generation;
		}
		else
		{
			// Standalone usage (no Polyphony/MIDI source upstream): use the
			// trigger input itself as a gate signal. Positive values hold the
			// note; a rising edge (false -> true) starts a new generation so
			// the envelope retriggers just like a fresh voice assignment.
			bool held = triggerValue > 0.0;
			if (held && !standaloneHeld)
				standaloneGeneration++;
			standaloneHeld = held;
			noteHeld = held;
			gen = standaloneGeneration;
		}

		// Detect new voice assignment
		if (gen != lastSeenGeneration && triggerValue != 0.0)
		{
			bool wasActive = (lastSeenGeneration != 0);
			lastSeenGeneration = gen;

			if (wasActive)
			{
				// Retrigger: keep current envelope state (amplitude, phase) but update
				// control points from reference in case they changed. The envelope's
				// Retrigger phase then ramps from the current amplitude to peak.
				for (int i = 0; i < 8; i++)
					voiceEnvelope.controlPoints[i] = reference.controlPoints[i];

				// Re-assigned while the gate is still high (e.g. a mono retrigger mode):
				// release for this sample so the GetAmplitude() call below sees a new
				// note-on, otherwise the envelope would stay in Sustain.
				if (noteHeld && voiceEnvelope.noteOn)
					voiceEnvelope.GetAmplitude(time, false);
			}
			else
			{
				// Fresh voice (never used or fully deactivated): start clean
				voiceEnvelope = reference;
			}
		}

		double amplitude = voiceEnvelope.GetAmplitude(time, noteHeld);

		outputValues[0] = inputValue * amplitude;
	}
};
