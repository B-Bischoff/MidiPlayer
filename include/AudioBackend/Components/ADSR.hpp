#pragma once

#include "AudioComponent.hpp"
#include "audio_backend.hpp"
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
	std::vector<EnvelopeInfo> envelopes;

	ADSR() : AudioComponent() { inputs.resize(2); }

	double process(std::vector<MidiInfo>& keyPressed, int currentKey = 0) override
	{
		if (!inputs.size())
			return 0.0;

		double inputValue = getInputsValue(input, keyPressed, currentKey);
		double triggerValue = getInputsValue(trigger, keyPressed, currentKey);

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
					// As the note must be extended, it is no longer in the keyPressed vector.
					// This runs the pipeline from this ADSR node using a new keyPressed vector containing
					// only the release note as if it were played by the user's midi keyboard.
					std::vector<MidiInfo> newKeyPressed;
					if (envelopeInfo.info.keyIndex != 0)
						newKeyPressed.push_back(envelopeInfo.info);
					inputValue = getInputsValue(input, newKeyPressed, 0);
					value += envelopeInfo.envelope.GetAmplitude(time, false) * inputValue;
				}
			}
		}

		// remove finished envelopes
		for (auto it = envelopes.begin(); it != envelopes.end(); it++)
		{
			//if (it->phase == Phase::Inactive)
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

