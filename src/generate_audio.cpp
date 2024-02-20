#include "inc.hpp"
#include "envelope.hpp"

static double pianoKeyFrequency(int keyId);
static double osc(double hertz, double time);

// [TODO] think to group everything (or not?) in a class/struct
void generateAudio(AudioData& audio, InputManager& inputManager, std::vector<sEnvelopeADSR>& envelopes, double& time)
{
	static int TEST = 0;
	double samplesPerFrame = (double)audio.sampleRate / (double)audio.targetFPS;
	double fractionalPart = samplesPerFrame - (int)samplesPerFrame;
	int complementaryFrame = 0;
	bool writeOneMoreFrame = false;
	if (fractionalPart != 0.0)
	{
		complementaryFrame = std::ceil(1.0 / fractionalPart);
		writeOneMoreFrame = TEST % complementaryFrame == 0;
	}
	TEST++;
	//std::cout << writeOneMoreFrame << std::endl;
	//for (int i = 0; i < audio.sampleRate / audio.targetFPS + (int)writeOneMoreFrame; i++)
	for (int i = 0; i < audio.sampleRate / audio.targetFPS + writeOneMoreFrame; i++)
	{
		//std::cout << " >>> " << audio.sampleRate / audio.targetFPS + (i % 3 == 0) << std::endl;
		double tmp;
		//double t = programElapsedTime.count() + (1.0f / (double)audio.sampleRate * (double)(i));
		double value = 0.0;

		for (sEnvelopeADSR& e : envelopes)
		{
			if (e.noteOn || e.phase != Phase::Inactive)
				value += osc(pianoKeyFrequency(e.keyIndex), time) * 0.3 * e.GetAmplitude(time);
		}

		//if (writeOneMoreFrame && i == audio.sampleRate/audio.targetFPS)
		//	value = 0;
		//std::cout << t << " " << value << " " << (writeOneMoreFrame && i == audio.sampleRate/audio.targetFPS ? "2" : "1") << std::endl;
		//t += (double)audio.sampleRate / (double)audio.targetFPS;
		time += 1.0 / (double)audio.sampleRate;

		for (int j = 0; j < audio.channels; j++)
		{
			audio.buffer[audio.writeCursor] = value;
			audio.incrementWriteCursor();
		}
	}
}

static double pianoKeyFrequency(int keyId)
{

	// Frequency of key A4 (A440) is 440 Hz
	double A4Frequency = 440.0;

	// Number of keys from A4 to the given key
	int keysDifference = keyId - 49;

	// Frequency multiplier for each semitone
	double semitoneRatio = pow(2.0, 1.0/12.0);

	// Calculate the frequency of the given key
	double frequency = A4Frequency * pow(semitoneRatio, keysDifference);

	//std::cout << keyId << " " << frequency << std::endl;

	return frequency;
}

static double freqToAngularVelocity(double hertz)
{
	return hertz * 2.0 * M_PI;
}

static double osc(double hertz, double time)
{
	double t = sin(freqToAngularVelocity(hertz) * time);
	return t;
}
