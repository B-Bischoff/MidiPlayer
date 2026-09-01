#pragma once

#include "AudioComponent.hpp"
#include "AudioBackend/VoiceContext.hpp"
#include "envelope.hpp"

struct ADSR : public AudioComponent {
private:
	struct EnvelopeInfo {
		sEnvelopeADSR envelope;
		MidiInfo info;
		unsigned int id;
		bool playedThisFrame = false;
	};
public:
	enum Inputs { input, trigger };

	sEnvelopeADSR reference; // Used to store envelope settings value
	std::vector<EnvelopeInfo> envelopes; // Used by non-polyphonic path

	// Used by polyphonic path (one envelope per clone)
	sEnvelopeADSR voiceEnvelope;
	unsigned int lastSeenGeneration = 0;

	ADSR() : AudioComponent() { inputs.resize(2); componentName = "ADSR"; }

	std::shared_ptr<AudioComponent> clone() const override {
		auto c = std::make_shared<ADSR>();
		c->reference = reference;
		return c;
	}

	double process(const AudioInfos& audioInfos, std::vector<MidiInfo>& keyPressed, int currentKey = 0) override
	{
		if (!inputs.size())
			return 0.0;

		// Polyphonic path: single envelope per clone, driven by VoiceContext
		if (activeVoiceContext)
			return processPolyphonic(audioInfos, keyPressed);

		// Non-polyphonic path: original multi-envelope system
		return processLegacy(audioInfos, keyPressed, currentKey);
	}

private:
	double processPolyphonic(const AudioInfos& audioInfos, std::vector<MidiInfo>& keyPressed)
	{
		double inputValue = getInputsValue(input, audioInfos, keyPressed, 0);

		bool noteHeld = (activeVoiceContext->noteInfo.keyIndex != 0 && !activeVoiceContext->releasing);
		unsigned int gen = activeVoiceContext->generation;

		// Detect new voice assignment
		if (gen != lastSeenGeneration)
		{
			bool wasActive = (lastSeenGeneration != 0);
			lastSeenGeneration = gen;

			if (wasActive)
			{
				// Retrigger: keep current envelope state (amplitude, phase)
				// but update control points from reference in case they changed.
				// The envelope's built-in retrigger logic will ramp from the
				// current amplitude to peak.
				for (int i = 0; i < 8; i++)
					voiceEnvelope.controlPoints[i] = reference.controlPoints[i];
			}
			else
			{
				// Fresh voice (never used or fully deactivated): start clean
				voiceEnvelope = reference;
			}
		}

		double amplitude = voiceEnvelope.GetAmplitude(time, noteHeld);

		return inputValue * amplitude;
	}

	double processLegacy(const AudioInfos& audioInfos, std::vector<MidiInfo>& keyPressed, int currentKey)
	{
		double inputValue = getInputsValue(input, audioInfos, keyPressed, currentKey);
		double triggerValue = getInputsValue(trigger, audioInfos, keyPressed, currentKey);

		if (currentKey == 0)
		{
			for (auto& e : envelopes)
				e.playedThisFrame = false;
		}

		unsigned int envelopeIndex = keyPressed.size() ? keyPressed[currentKey].keyIndex : triggerValue;

		// add new envelopes
		if (envelopeIndex != 0.0  && triggerValue != 0.0)
		{
			bool envelopeAlreadyExists = false;
			for (EnvelopeInfo& envelopeInfo : envelopes)
			{
				if (envelopeInfo.id == envelopeIndex)
				{
					envelopeAlreadyExists = true;
					break;
				}
			}
			if (!envelopeAlreadyExists)
			{
				EnvelopeInfo envelopeInfo;
				envelopeInfo.id = envelopeIndex;
				envelopeInfo.info = {};
				envelopeInfo.envelope = reference;
				if (keyPressed.size())
					envelopeInfo.info = keyPressed[currentKey];

				envelopes.push_back(envelopeInfo);
			}
		}

		double value = 0.0;

		// update envelopes
		for (EnvelopeInfo& envelopeInfo : envelopes)
		{
			if (envelopeInfo.id == envelopeIndex && triggerValue != 0.0)
			{
				value += inputValue * envelopeInfo.envelope.GetAmplitude(time, true);
				envelopeInfo.playedThisFrame = true;
				break;
			}
		}

		// Play envelope in release
		if (currentKey == keyPressed.size() - 1 || keyPressed.empty())
		{
			for (EnvelopeInfo& envelopeInfo : envelopes)
			{
				if (!envelopeInfo.playedThisFrame)
				{
					std::vector<MidiInfo> newKeyPressed;
					if (envelopeInfo.info.keyIndex != 0)
						newKeyPressed.push_back(envelopeInfo.info);
					inputValue = getInputsValue(input, audioInfos, newKeyPressed, 0);
					value += envelopeInfo.envelope.GetAmplitude(time, false) * inputValue;
				}
			}
		}

		// remove finished envelopes
		for (auto it = envelopes.begin(); it != envelopes.end(); it++)
		{
			if (it->envelope.phase == Phase::Inactive)
			{
				it = envelopes.erase(it);
				if (it == envelopes.end())
					break;
			}
		}

		return value;
	}
};
