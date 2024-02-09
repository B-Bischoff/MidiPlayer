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


// Amplitude (Attack, Decay, Sustain, Release) Envelope
struct sEnvelopeADSR
{
	double dAttackTime;
	double dDecayTime;
	double dSustainAmplitude;
	double dReleaseTime;
	double dStartAmplitude;
	double dTriggerOffTime;
	double dTriggerOnTime;
	bool bNoteOn;

	sEnvelopeADSR()
	{
		dAttackTime = .20;
		dDecayTime = 0.10;
		dStartAmplitude = 1.0;
		dSustainAmplitude = 0.8;
		dReleaseTime = 0.20;
		bNoteOn = false;
		dTriggerOffTime = 0.0;
		dTriggerOnTime = 0.0;
	}

	// Call when key is pressed
	void NoteOn(double dTimeOn)
	{
		std::cout << "note on " << dTimeOn << std::endl;
		dTriggerOnTime = dTimeOn;
		bNoteOn = true;
	}

	// Call when key is released
	void NoteOff(double dTimeOff)
	{
		std::cout << "note off " << dTimeOff << std::endl;
		dTriggerOffTime = dTimeOff;
		bNoteOn = false;
	}

	// Get the correct amplitude at the requested point in time
	double GetAmplitude(double dTime)
	{
		double dAmplitude = 0.0;
		double dLifeTime = dTime - dTriggerOnTime;

		if (dTriggerOnTime == 0.0f)
			return 0;

		if (bNoteOn)
		{
			if (dLifeTime <= dAttackTime)
			{
				// In attack Phase - approach max amplitude
				dAmplitude = (dLifeTime / dAttackTime) * dStartAmplitude;
			}

			if (dLifeTime > dAttackTime && dLifeTime <= (dAttackTime + dDecayTime))
			{
				// In decay phase - reduce to sustained amplitude
				dAmplitude = ((dLifeTime - dAttackTime) / dDecayTime) * (dSustainAmplitude - dStartAmplitude) + dStartAmplitude;
			}

			if (dLifeTime > (dAttackTime + dDecayTime))
			{
				// In sustain phase - dont change until note released
				dAmplitude = dSustainAmplitude;
			}
		}
		else
		{
			// Note has been released, so in release phase
			dAmplitude = ((dTime - dTriggerOffTime) / dReleaseTime) * (0.0 - dSustainAmplitude) + dSustainAmplitude;
		}

		// Amplitude should not be negative
		if (dAmplitude <= 0.0001)
			dAmplitude = 0.0;

		return dAmplitude;
	}
};

sEnvelopeADSR e;

double freqToAngularVelocity(double hertz)
{
	return hertz * 2.0 * M_PI;
}

double ocs(double hertz, double time)
{
	double t = sin(freqToAngularVelocity(hertz) * time);
	return t;
}

struct InputManager {
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

	//std::cout << "callback time : " << streamTime << std::endl;
	if (status)
		std::cout << "Stream underflow detected!" << std::endl;

	//std::cout << nBufferFrames << " | " << rb.writeCursor << " " << rb.leftPhase << std::endl;
	for (int i = 0 ; i<nBufferFrames; i++)
	{
		double tmp;
		*buffer++ = audio.buffer[audio.leftPhase];
		if (audio.channels == 2)
			*buffer++ = audio.buffer[audio.rightPhase];
		audio.incrementPhases();
	}
	//std::cout << audio.leftPhase / 2 << " " << audio.writeCursor / 2.0f << std::endl;

	return 0;
}

int main(void)
{
	AudioData audio = {
		.sampleRate = 44100,
		.channels = 2,
		.bufferDuration = 2,
		.latency = 2,
		.buffer = nullptr,
		.leftPhase = 0, .rightPhase = 1,
		.writeCursor = 0,
		.targetFPS = 60,
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
	std::cout << "L/R/W : " << audio.leftPhase << " " << audio.rightPhase << " " << audio.writeCursor << std::endl;
	while (1)
	{
		auto startTime = std::chrono::high_resolution_clock::now();
		auto programElapsedTime = std::chrono::duration_cast<std::chrono::duration<double>>(startTime - audio.startTime);
		//std::cout << programElapsedTime.count() << std::endl;

		// INPUTS
		int numEvents = Pm_Read(inputManager.midiStream, inputManager.buffer, 32);

		for (int i = 0; i < numEvents; i++)
		{
			PmEvent& event = inputManager.buffer[i];

			// Extract MIDI status and data bytes
			PmMessage message = event.message;
			int status = Pm_MessageStatus(message);
			int data1 = Pm_MessageData1(message);
			int data2 = Pm_MessageData2(message);

			std::cout << " state " << status << " key " << (int)data1 << " vel " << (int)data2 << std::endl;
			audio.test = status == 145;
			if (status == 145)
				e.NoteOn(programElapsedTime.count());
			else
				e.NoteOff(programElapsedTime.count());
		}

		// AUDIO

		for (int i = 0; i < audio.sampleRate / audio.targetFPS; i++)
		{
			double tmp;
			double t = programElapsedTime.count() + (1.0f / (double)audio.sampleRate * (double)(i));
			double value = ocs(440, t) * 0.5 * e.GetAmplitude(programElapsedTime.count());

			for (int j = 0; j < audio.channels; j++)
			{
				audio.buffer[audio.writeCursor] = value;
				audio.incrementWriteCursor();
			}
		}
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
