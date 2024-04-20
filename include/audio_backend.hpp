#pragma once

#include "envelope.hpp"
#include "inc.hpp"

double osc(double hertz, double time, OscType type = Sine, double LFOHertz = 0.0, double LFOAmplitude = 0.0);
double pianoKeyFrequency(int keyId);

struct AudioComponent {
	AudioComponent() : input(nullptr) { }
	virtual ~AudioComponent() {};
	virtual double process(std::vector<MidiInfo>& keyPressed, int currentKey = 0) = 0;

	static double time;
	AudioComponent* input;
};

struct Master : public AudioComponent {
private:
	bool showWarning = true;

public:
	double process(std::vector<MidiInfo>& keyPressed, int currentKey = 0)
	{
		if (!input)
		{
			if (showWarning)
			{
				showWarning = false;
				std::cout << "[WARNING] no input plugged in master" << std::endl;
			}
			return 0;
		}

		showWarning = true;

		double value = 0.0;


		int i = 0;
		do
		{
			value += input->process(keyPressed, i);
		} while (++i < keyPressed.size());

		return value;
	}
};

struct KeyboardFrequency : public AudioComponent {
	static unsigned int keyIndex;

	double process(std::vector<MidiInfo>& keyPressed, int currentKey = 0)
	{
		if (!keyPressed.size())
			return 0.0;

		return pianoKeyFrequency(keyPressed[currentKey].keyIndex);
	}
};

struct ADSR : public AudioComponent {
	sEnvelopeADSR reference; // Used to store envelope settings value
	std::vector<sEnvelopeADSR> envelopes;

	double process(std::vector<MidiInfo>& keyPressed, int currentKey = 0)
	{
		if (!input)
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
				std::cout << "erasing " << it->keyIndex << std::endl;
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

		// Play note in release (because they do not appear in keyPressed)
		if (currentKey == 0)
		{
			for (sEnvelopeADSR& envelope : envelopes)
			{
				if (!envelope.noteOn)
				{
					//std::cout << "playing release note" << std::endl;
					std::vector<MidiInfo> tempKeyPressed;
					MidiInfo info = {
						.keyIndex = (int)envelope.keyIndex,
						.velocity = 0, // TO DEFINE (OR TO STORE BEFORE NOTE GETS OFF)
						.risingEdge = false,
					};
					tempKeyPressed.push_back(info);
					value += input->process(tempKeyPressed, 0) * envelope.GetAmplitude(time);
				}
			}
		}

		if (keyPressed.size())
		{
			// Find envelope corresponding to keyPressed[currentKey]
			for (sEnvelopeADSR& envelope : envelopes)
			{
				if (keyPressed[currentKey].keyIndex == envelope.keyIndex)
				{
					value += input->process(keyPressed, currentKey) * envelope.GetAmplitude(time);

					return value;
				}
			}
		}

		return value;
	}
};

struct Oscillator : public AudioComponent {
	OscType type;

	double process(std::vector<MidiInfo>& keyPressed, int currentKey = 0)
	{
		if (!input)
			return 0.0;

		double frequency = input->process(keyPressed, currentKey);
		double value = osc(frequency, time, type);
		return value;
	}
};
