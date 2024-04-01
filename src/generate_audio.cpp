#include "inc.hpp"
#include "audio_backend.hpp"

void applyLowPassFilter(double& sample)
{
	static double lowPassState = 0.0;
	double lowPassAlpha = 0.01;
	lowPassState = lowPassAlpha * sample + (1.0 - lowPassAlpha) * lowPassState;
	sample = lowPassState;
}

// [TODO] think to group everything (or not?) in a class/struct
void generateAudio(AudioData& audio, Master& master, std::vector<sEnvelopeADSR>& envelopes, double& time)
{
	static int TEST = 0;
	double fractionalPart = audio.getFramesPerUpdate() - (int)audio.getFramesPerUpdate();
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

	for (int i = 0; i < (int)audio.getFramesPerUpdate() + writeOneMoreFrame + audio.samplesToAdjust; i++)
	{
		//std::cout << " >>> " << audio.sampleRate / audio.targetFPS + (i % 3 == 0) << std::endl;
		double tmp;
		//double t = programElapsedTime.count() + (1.0f / (double)audio.sampleRate * (double)(i));
		double value = 0.0;

		for (sEnvelopeADSR& e : envelopes)
		{
			if (e.noteOn || e.phase != Phase::Inactive)
			{
				//double amplitude = e.GetAmplitude(time);
				KeyboardFrequency::keyIndex = e.keyIndex;
				ADSR::envelope = &e;

				//for (int j = 1; j < 10; j++)
				//	value += osc(pianoKeyFrequency(e.keyIndex) * j, time) * 0.2 * (1.0 / (double)j) * amplitude;

				//value += osc(pianoKeyFrequency(e.keyIndex), time, 20.0, 0.02) * 0.3 * e.GetAmplitude(time);
				//value += osc(pianoKeyFrequency(e.keyIndex), time) * 0.3 * e.GetAmplitude(time);
				value += master.process() * 0.3;// * e.GetAmplitude(time);
				//applyLowPassFilter(value);
			}
		}

		//if (writeOneMoreFrame && i == audio.sampleRate/audio.targetFPS)
		//	value = 0;
		//std::cout << "TIME : " << time << " " << value << " " << std::endl;
		//t += (double)audio.sampleRate / (double)audio.targetFPS;
		time += 1.0 / (double)audio.sampleRate;
		Base::time = time;

		for (int j = 0; j < audio.channels; j++)
		{
			audio.buffer[audio.writeCursor] = value;
			audio.incrementWriteCursor();
		}
	}

	assert(audio.syncCursors == false && "Audio callback did not reset syncCursors");
	audio.syncCursors = true;
}

double pianoKeyFrequency(int keyId)
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

double osc(double hertz, double time, double LFOHertz, double LFOAmplitude)
{
	double t = freqToAngularVelocity(hertz) * time + LFOAmplitude * hertz * (sin(freqToAngularVelocity(LFOHertz) * time));
	t = sin(t); // SINE oscillator
	return t;
}
