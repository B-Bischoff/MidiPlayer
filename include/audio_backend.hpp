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

		double value = input->process(keyPressed);
		return value;
	}
};

struct KeyboardFrequency : public AudioComponent {
	static unsigned int keyIndex;

	double process(std::vector<MidiInfo>& keyPressed, int currentKey = 0)
	{
		return pianoKeyFrequency(currentKey);
		//return pianoKeyFrequency(keyIndex);
	}
};

struct ADSR : public AudioComponent {
	sEnvelopeADSR reference; // Used to store envelope settings value
	std::vector<sEnvelopeADSR> envelopes;

	double process(std::vector<MidiInfo>& keyPressed, int currentKey = 0)
	{
		if (!input)
			return 0.0;

		//double value = input->process(keyPressed) * envelope->GetAmplitude(time);


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
						std::cout << "Retrigger : " << envelope.keyIndex << std::endl;
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

		// Loop on envelopes
		for (int i = 0; i < envelopes.size(); i++)
		{
			value += input->process(keyPressed, envelopes[i].keyIndex) * envelopes[i].GetAmplitude(time);
		}

		//for (int i = 0; i < envelopes.size(); i++)

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
