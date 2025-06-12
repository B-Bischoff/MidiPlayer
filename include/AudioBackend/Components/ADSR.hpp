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

	ADSR() : AudioComponent() { inputs.resize(2); componentName = "ADSR"; }

	double process(PipelineInfo& info) override
	{
		if (!inputs.size())
			return 0.0;

		double triggerValue = getInputsValue(trigger, info);

		if (info.currentKey == 0)
		{
			for (auto& e : envelopes)
				e.playedThisFrame = false;
		}

		unsigned int envelopeIndex = info.keyPressed.size() ? info.keyPressed[info.currentKey].keyIndex : triggerValue;

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
				if (info.keyPressed.size())
					envelopeInfo.info = info.keyPressed[info.currentKey];

				envelopes.push_back(envelopeInfo);
			}
		}

		double value = 0.0;

		// Play envelope in release
		double relaseEnvelopeInputValue = 0.0;
		if (info.currentKey == info.keyPressed.size() - 1 || info.keyPressed.empty())
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
					PipelineInfo newPipeline(newKeyPressed);
					newPipeline.pipelineInitiator = 0;
					newPipeline.currentKey = 0;
					relaseEnvelopeInputValue = getInputsValue(input, newPipeline);
					value += envelopeInfo.envelope.GetAmplitude(time, false) * relaseEnvelopeInputValue;
				}
			}
		}

		// update envelopes
		double inputValue = getInputsValue(input, info) + relaseEnvelopeInputValue;
		for (EnvelopeInfo& envelopeInfo : envelopes)
		{
			if (envelopeInfo.id == envelopeIndex && triggerValue != 0.0)
			{
				value += inputValue * envelopeInfo.envelope.GetAmplitude(time, true);
				envelopeInfo.playedThisFrame = true;
				break;
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

