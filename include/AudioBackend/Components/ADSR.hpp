#pragma once

#include "AudioComponent.hpp"
#include "audio_backend.hpp"
#include "envelope.hpp"

struct ADSR : public AudioComponent {
	sEnvelopeADSR reference; // Used to store envelope settings value
	std::vector<sEnvelopeADSR> envelopes;
	std::vector<AudioComponent*> inputs;

	Components getInputs() override
	{
		return combineVectorsToForwardList(inputs);
	}

	void clearInputs() override
	{
		clearVectors(inputs);
	}

	void addInput(const std::string& inputName, AudioComponent* input) override
	{
		inputs.push_back(input);
	}

	double process(std::vector<MidiInfo>& keyPressed, int currentKey = 0) override
	{
		if (!inputs.size())
			return 0.0;

		// When key is no longer on, set envelope notOff
		for (sEnvelopeADSR& envelope : envelopes)
		{
			if (!envelope.noteOn) // Note is already off
				continue;

			bool notePressed = false;
			for (auto it = keyPressed.begin(); it != keyPressed.end(); it++)
			{
				if (it->keyIndex == envelope.keyIndex)
					notePressed = true;
			}

			if (!notePressed)
				envelope.NoteOff(time);
		}

		// Erase finished envelopes
		for (auto it = envelopes.begin(); it != envelopes.end(); it++)
		{
			if (it->phase == Phase::Inactive)
			{
				it = envelopes.erase(it);
				if (it == envelopes.end())
					break;
			}
		}

		// Create new envelopes on key pressed
		for (MidiInfo& info : keyPressed)
		{
			bool keyIdInEnvelopes = false;
			for (sEnvelopeADSR& envelope : envelopes)
			{
				if (info.keyIndex == envelope.keyIndex)
				{
					keyIdInEnvelopes = true;

					if (info.risingEdge)
					{
						envelope.NoteOn(time);
					}
				}
			}
			if (!keyIdInEnvelopes) // Add new envelope
			{
				sEnvelopeADSR envelope;
				envelope.keyIndex = info.keyIndex;
				envelopes.push_back(envelope); // [TODO] move getOn code in constructor
				envelopes.back().NoteOn(time);
			}
		}

		double value = 0.0;

		// Play note in release because they do not appear in keyPressed
		if (currentKey == 0)
		{
			for (sEnvelopeADSR& envelope : envelopes)
			{
				// Make sure note is not played if its in keyPressed
				// This may happen if the note is pressed right after it is relesed
				bool isInKeyPressed = false;
				for (MidiInfo& info : keyPressed)
					if (info.keyIndex == envelope.keyIndex)
						isInKeyPressed = true;

				if (!envelope.noteOn && !isInKeyPressed)
				{
					//std::cout << "playing release note" << std::endl;
					std::vector<MidiInfo> tempKeyPressed;
					MidiInfo info = {
						.keyIndex = (int)envelope.keyIndex,
						.velocity = 0, // TO DEFINE (OR TO STORE BEFORE NOTE GETS OFF)
						.risingEdge = false,
					};
					tempKeyPressed.push_back(info);
					for (AudioComponent* input : inputs)
						value += input->process(tempKeyPressed, 0) * envelope.GetAmplitude(time);
				}
			}
		}

		if (keyPressed.size())
		{
			for (AudioComponent* input : inputs)
			{
				// Find envelope corresponding to keyPressed[currentKey]
				for (sEnvelopeADSR& envelope : envelopes)
				{
					if (keyPressed[currentKey].keyIndex == envelope.keyIndex)
					{
						value += input->process(keyPressed, currentKey) * envelope.GetAmplitude(time);
						break;
					}
				}
			}
		}

		return value;
	}
};

