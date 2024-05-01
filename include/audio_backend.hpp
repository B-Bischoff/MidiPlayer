#pragma once

#include "envelope.hpp"
#include "inc.hpp"

double osc(double hertz, double time, OscType type = Sine, double LFOHertz = 0.0, double LFOAmplitude = 0.0);
double pianoKeyFrequency(int keyId);

struct AudioComponent {
	AudioComponent() { }
	virtual ~AudioComponent() {};

	virtual double process(std::vector<MidiInfo>& keyPressed, int currentKey = 0) = 0;
	virtual Components getInputs() { return Components(); }
	virtual void clearInputs() { }
	virtual void addInput(const std::string& inputName, AudioComponent* input) {
		assert(0 && "Node does not have input");
	}
	virtual double getInputsValue(std::vector<AudioComponent*>& inputs, std::vector<MidiInfo>& keyPressed, int currentKey = 0) {
		double value = 0.0;
		for (AudioComponent* input : inputs)
			value += input->process(keyPressed, currentKey);
		return value;
	}

	static double time;
};

struct Master : public AudioComponent {
private:
	bool showWarning = true;

public:
	std::vector<AudioComponent*> inputs;

	Components getInputs() override
	{
		Components components;
		for (AudioComponent* input : inputs)
			components.push_front(input);
		return components;
	}

	void addInput(const std::string& inputName, AudioComponent* input) override
	{
		// [TODO] check inputName even thought it is not necessary for 1 input nodes ?
		inputs.push_back(input);
	}

	void clearInputs() override
	{
		inputs.clear();
	}

	double process(std::vector<MidiInfo>& keyPressed, int currentKey = 0) override
	{
		if (!inputs.size())
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


		for (AudioComponent* input : inputs)
		{
			int i = 0;
			do
			{
				value += input->process(keyPressed, i);
			} while (++i < keyPressed.size());
		}

		return value;
	}
};

struct KeyboardFrequency : public AudioComponent {
	static unsigned int keyIndex;

	double process(std::vector<MidiInfo>& keyPressed, int currentKey = 0) override
	{
		if (!keyPressed.size())
			return 0.0;

		return pianoKeyFrequency(keyPressed[currentKey].keyIndex);
	}
};

struct ADSR : public AudioComponent {
	sEnvelopeADSR reference; // Used to store envelope settings value
	std::vector<sEnvelopeADSR> envelopes;
	std::vector<AudioComponent*> inputs;

	Components getInputs() override
	{
		Components components;
		for (AudioComponent* input : inputs)
			components.push_front(input);
		return components;
	}

	void clearInputs() override
	{
		inputs.clear();
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

struct Number : public AudioComponent {
	float number;

	double process(std::vector<MidiInfo>& keyPressed, int currentKey = 0) override
	{
		//std::cout << "returning " << number << std::endl;
		return number;
	}
};

struct Oscillator : public AudioComponent {
	OscType type;
	std::vector<AudioComponent*> freq;
	std::vector<AudioComponent*> LFO_Hz;
	std::vector<AudioComponent*> LFO_Amplitude;

	Components getInputs() override
	{
		Components components;
		for (AudioComponent* input : freq)
			components.push_front(input);
		for (AudioComponent* input : LFO_Hz)
			components.push_front(input);
		for (AudioComponent* input : LFO_Amplitude)
			components.push_front(input);
		return components;
	}

	void clearInputs() override
	{
		freq.clear();
		LFO_Hz.clear();
		LFO_Amplitude.clear();
	}

	void addInput(const std::string& inputName, AudioComponent* input) override
	{
		if (inputName == "> freq") // [TODO] remove those "> "
			freq.push_back(input);
		else if (inputName == "> LFO Hz")
			LFO_Hz.push_back(input);
		else if (inputName == "> LFO Amplitude")
			LFO_Amplitude.push_back(input);
		else
			assert(0 && "[Oscillator node] unknown input");
	}

	double process(std::vector<MidiInfo>& keyPressed, int currentKey = 0) override
	{
		if (!freq.size())
			return 0.0;
		double frequencyValue = getInputsValue(freq, keyPressed, currentKey);
		double LFO_Hz_Value = getInputsValue(LFO_Hz, keyPressed, currentKey);
		double LFO_Amplitude_Value = getInputsValue(LFO_Amplitude, keyPressed, currentKey);
		double value = osc(frequencyValue, time, type, LFO_Hz_Value, LFO_Amplitude_Value);

		return value;
	}
};

struct Multiplier : public AudioComponent {
	std::vector<AudioComponent*> inputsA;
	std::vector<AudioComponent*> inputsB;

	Components getInputs() override
	{
		Components components;
		for (AudioComponent* input : inputsA)
			components.push_front(input);
		for (AudioComponent* input : inputsB)
			components.push_front(input);
		return components;
	}

	void clearInputs() override
	{
		inputsA.clear();
		inputsB.clear();
	}

	void addInput(const std::string& inputName, AudioComponent* input) override
	{
		if (inputName == "> input A") // [TODO] remove those "> "
			inputsA.push_back(input);
		else if (inputName == "> input B")
			inputsB.push_back(input);
		else
			assert(0 && "[Oscillator node] unknown input");
	}

	double process(std::vector<MidiInfo>& keyPressed, int currentKey = 0) override
	{
		double valueA = getInputsValue(inputsA, keyPressed, currentKey);
		double valueB = getInputsValue(inputsB, keyPressed, currentKey);
		return valueA * valueB;
	}
};
