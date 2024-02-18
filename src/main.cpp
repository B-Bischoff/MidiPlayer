#include <algorithm>
#include <sys/types.h>
#if __has_include("RtAudio.h")
#include <RtAudio.h>
#else
#include <rtaudio/RtAudio.h> // Development purpose
#endif

// includes for development purpose, to remove
#include <cmath>
#include <cstring>
#include <iostream>
#include <math.h>
#include <chrono>
#include <thread>
#include <assert.h>
#include <portmidi.h>

#include "inc.hpp"
#include "../include/inc.hpp"

#ifdef _WIN32
#include <conio.h>
#else
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#endif

enum Phase { Attack, Decay, Sustain, Release, Inactive, Retrigger };

struct sEnvelopeADSR
{

	unsigned int keyIndex;

	double attackTime;
	double decayTime;
	double sustainAmplitude;
	double releaseTime;
	double attackAmplitude;
	double triggerOffTime;
	double triggerOnTime;
	bool noteOn;

	bool retrigger;
	double amplitudeBeforeRetrigger;
	double amplitudeAtOffTrigger;

	Phase phase;
	double amplitude;

	sEnvelopeADSR()
	{
		attackTime = .05;
		decayTime = .05,
		attackAmplitude = 1.0;
		sustainAmplitude = 0.8;
		releaseTime = 0.05;
		noteOn = false;
		triggerOffTime = 0.0;
		triggerOnTime = 0.0;
		retrigger = false;
		phase = Inactive;
		amplitude = 0.0;
	}

	void NoteOn(double dTimeOn)
	{
		//std::cout << "note on " << dTimeOn << std::endl;
		triggerOnTime = dTimeOn;
		noteOn = true;
		phase = Phase::Attack;
	}

	void NoteOff(double dTimeOff)
	{
		//std::cout << "note off " << dTimeOff << std::endl;
		triggerOffTime = dTimeOff;
		noteOn = false;
	}

	Phase getPhase(double time)
	{
		if (!noteOn)
		{
			if (time - triggerOffTime <= releaseTime)
				return Phase::Release;
			else
				return Phase::Inactive;
		}

		const double lifeTime = time - triggerOnTime;

		if (triggerOffTime != 0.0 && triggerOnTime != 0.0f && triggerOnTime - triggerOffTime < releaseTime && lifeTime <= attackTime)
			return Phase::Retrigger;

		if (lifeTime <= attackTime)
			return Phase::Attack;
		else if (lifeTime <= attackTime + decayTime)
			return Phase::Decay;
		else
			return Phase::Sustain;
	}

	double GetAmplitude(double time)
	{
		if (triggerOnTime == 0.0f)
			return 0;

		double lifeTime = time - triggerOnTime;

		Phase newPhase = getPhase(time);

		if (newPhase == Retrigger && phase == Release) // Press key while its release phase is not finished
			amplitudeBeforeRetrigger = amplitude;
		else if (newPhase == Release && phase != Release) // Save last amplitude before note off
			amplitudeAtOffTrigger = amplitude;

		phase = newPhase;

		switch (phase)
		{
			case Phase::Attack :
				amplitude = (lifeTime / attackTime) * attackAmplitude;
				break;

			case Phase::Decay :
				retrigger = false;
				lifeTime -= attackTime;
				amplitude = (lifeTime / decayTime) * (sustainAmplitude - attackAmplitude) + attackAmplitude;
				break;

			case Phase::Sustain :
				amplitude = sustainAmplitude;
				break;

			case Phase::Release :
				lifeTime = time - triggerOffTime;
				std::cout << ">>> " << lifeTime << " " << amplitudeAtOffTrigger << std::endl;
				amplitude = (1.0 - (lifeTime / releaseTime)) * amplitudeAtOffTrigger;
				break;

			case Phase::Inactive :
				amplitude = 0.0;
				keyIndex = 0;
				triggerOnTime = 0;
				triggerOffTime = 0;
				amplitudeAtOffTrigger = 0;
				break;

			case Phase::Retrigger :
				amplitude = (lifeTime / attackTime) * (attackAmplitude - amplitudeBeforeRetrigger) + amplitudeBeforeRetrigger;
				break;
		}


		if (amplitude <= 0.0001)
			amplitude = 0.0;

		//std::cout << time << " " << amplitude << " " << (retrigger ? 6 : phase) << std::endl;

		return amplitude;
	}
};

double freqToAngularVelocity(double hertz)
{
	return hertz * 2.0 * M_PI;
}

double osc(double hertz, double time)
{
	double t = sin(freqToAngularVelocity(hertz) * time);
	return t;
}

struct InputManager
{
	PmStream* midiStream;
	PmEvent buffer[32];
};

void initInput(InputManager& inputManger) // [TODO] Should this be in a constructor ?
{
	Pm_Initialize();

	int numDevices = Pm_CountDevices();
	if (numDevices <= 0)
	{
		std::cerr << "No MIDI devices found." << std::endl;
		exit(1);
	}

	for (int i = 0; i < numDevices; i++)
	{
		const PmDeviceInfo* info = Pm_GetDeviceInfo(i);
		assert(info);
#if VERBOSE
		std::cout << i << " " << info->name << std::endl;
		std::cout << "input/output: " << info->input << " " << info->output << std::endl;
		std::cout << "open/virtual: " << info->opened << " " << info->is_virtual << std::endl;
		std::cout << std::endl;
#endif
	}

	Pm_OpenInput(&inputManger.midiStream, 3, NULL, 512, NULL, NULL);
}

int uploadBuffer( void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
				double streamTime, RtAudioStreamStatus status, void *userData )
{
	double *buffer = (double *) outputBuffer;
	AudioData& audio = *(AudioData*)userData;

	static int SUM_FRAMES = 0;
	static int FRAME_COUNT = 0;
	FRAME_COUNT++;
	SUM_FRAMES += nBufferFrames;
	//std::cout << FRAME_COUNT << " : " << streamTime << " " << SUM_FRAMES << std::endl;

	//std::cout << "callback time : " << streamTime << std::endl;
	if (status)
		std::cout << "Stream underflow detected!" << std::endl;

	//std::cout << "buffer frames " << nBufferFrames << std::endl;

	for (int i = 0 ; i < nBufferFrames; i++)
	{
		double tmp;
		*buffer++ = audio.buffer[audio.leftPhase];
		if (audio.channels == 2)
			*buffer++ = audio.buffer[audio.rightPhase];
		audio.incrementPhases();
	}

	std::cout << audio.leftPhase << " " << audio.writeCursor << std::endl;

	return 0;
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

int main(void)
{
	std::vector<sEnvelopeADSR> envelopes(16);

	AudioData audio = {
		.sampleRate = 44100,
		.channels = 1,
		.bufferDuration = 2,
		.latency = 2,
		.buffer = nullptr,
		.leftPhase = 0, .rightPhase = 1,
		.writeCursor = 0,
		.targetFPS = 64,
	};

	const std::chrono::duration<double> targetFrameDuration(1.0f / (double)audio.targetFPS);

	audio.buffer = new float[(int)(audio.sampleRate * audio.bufferDuration) * audio.channels];

	if (audio.buffer == nullptr)
		exitError("[ERROR]: ring buffer allocation failed.");

	memset(audio.buffer, 0, sizeof(float) * audio.getBufferSize());

	rtAudioInit(audio);

	InputManager inputManager;
	initInput(inputManager);

	audio.startTime = std::chrono::high_resolution_clock::now();

	std::this_thread::sleep_for(std::chrono::milliseconds(100)); // let rtaudio get more stable [TODO] check if that is necessary
	audio.writeCursor = audio.leftPhase + ((double)audio.sampleRate / (double)audio.targetFPS) * audio.latency;
	//std::cout << "L/R/W : " << audio.leftPhase << " " << audio.rightPhase << " " << audio.writeCursor << std::endl;

	double t = 0.0;

	while (1)
	{
		auto startTime = std::chrono::high_resolution_clock::now();
		auto programElapsedTime = std::chrono::duration_cast<std::chrono::duration<double>>(startTime - audio.startTime);
		//std::cout << programElapsedTime.count() << " " << t << std::endl;

		// INPUTS
		if (USE_KB_AS_MIDI_INPUT)
		{
		}
		int numEvents = Pm_Read(inputManager.midiStream, inputManager.buffer, 32);

		for (int i = 0; i < numEvents; i++)
		{
			PmEvent& event = inputManager.buffer[i];

			// Extract MIDI status and data bytes
			PmMessage message = event.message;
			int status = Pm_MessageStatus(message);
			int keyIndex = Pm_MessageData1(message);
			int velocity = Pm_MessageData2(message);

			//std::cout << " state " << status << " key " << (int)keyIndex << " vel " << (int)velocity << std::endl;
			audio.test = status == 145;
			if ((status == 145 || status == 155) && velocity != 0.0)
			{
				for (sEnvelopeADSR& e : envelopes)
				{
					if (e.phase == Phase::Inactive)
					{
						//std::cout << keyIndex << " on " << std::endl;
						e.NoteOn(t);
						e.keyIndex = keyIndex;
						break;
					}
				}
			}
			else
			{
				for (sEnvelopeADSR& e : envelopes)
				{
					if (e.keyIndex == keyIndex)
					{
						//std::cout << keyIndex << " off " << std::endl;
						e.NoteOff(t);
						break;
					}
				}
			}
		}

		// AUDIO

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
					value += osc(pianoKeyFrequency(e.keyIndex), t) * 0.3 * e.GetAmplitude(t);
			}


			//if (writeOneMoreFrame && i == audio.sampleRate/audio.targetFPS)
			//	value = 0;
			//std::cout << t << " " << value << " " << (writeOneMoreFrame && i == audio.sampleRate/audio.targetFPS ? "2" : "1") << std::endl;
			//t += (double)audio.sampleRate / (double)audio.targetFPS;
			t += 1.0 / (double)audio.sampleRate;

			for (int j = 0; j < audio.channels; j++)
			{
				audio.buffer[audio.writeCursor] = value;
				audio.incrementWriteCursor();
			}
		}
		//std::cout << TEST << " " << audio.writeCursor << std::endl;
		//std::cout << "wrote " << rb.writeCursor << std::endl;

		// TIME MANAGEMENT

		auto endTime = std::chrono::high_resolution_clock::now();
		auto deltaTime = std::chrono::duration_cast<std::chrono::duration<double>>(endTime - startTime);

		auto sleepDuration = targetFrameDuration - deltaTime;

		if (sleepDuration > std::chrono::duration<double>(0.0))
		{
			std::this_thread::sleep_for(sleepDuration * 0.9f);
			endTime = std::chrono::high_resolution_clock::now();
			while (endTime - startTime < targetFrameDuration)
				endTime = std::chrono::high_resolution_clock::now();
		}
		else
			std::cerr << "[WARNING] : update took longer than expected" << std::endl;
	}

	// [TODO] should this be in destructor ?
	Pm_Close(inputManager.midiStream);
	Pm_Terminate();

	// [TODO] clean up audio
}
