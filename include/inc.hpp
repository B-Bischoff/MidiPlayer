#pragma once

// [TODO] Compile from source if not installed
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <chrono>
#include <portmidi.h>
#include <chrono>
#include <cmath>
#include <assert.h>
#include <thread>
#include <cstring>

#include <RtAudio.h>

#define VERBOSE true
#define USE_KB_AS_MIDI_INPUT true

typedef std::chrono::time_point<std::chrono::high_resolution_clock> time_point;

struct sEnvelopeADSR;

struct AudioData {
	unsigned int sampleRate;
	unsigned int channels;
	float bufferDuration; // In seconds
	unsigned int latency; // In frame

	float* buffer;
	unsigned int leftPhase, rightPhase;
	unsigned int writeCursor;

	unsigned int targetFPS;

	RtAudio stream;

	std::chrono::time_point<std::chrono::high_resolution_clock> startTime;

	unsigned int getBufferSize() const
	{
		return sampleRate * bufferDuration * channels;
	}

	void incrementPhases()
	{
		leftPhase = (leftPhase + channels) % (int(sampleRate * bufferDuration) * channels);
		rightPhase = (rightPhase + channels) % (int(sampleRate * bufferDuration) * channels);
	}

	void incrementWriteCursor()
	{
		writeCursor = (writeCursor + 1) % ((int)(sampleRate * bufferDuration) * channels);
	}
};

struct InputManager
{
	PmStream* midiStream;
	PmEvent buffer[32];
};

void rtAudioInit(AudioData& audio);
int uploadBuffer( void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status, void *userData);
void handleInput(InputManager& inputManager, std::vector<sEnvelopeADSR>& envelopes, double time);
void initInput(InputManager& inputManger);
void generateAudio(AudioData& audio, InputManager& inputManager, std::vector<sEnvelopeADSR>& envelopes, double& time);
int uploadBuffer(void *outputBuffer, void* inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status, void *userData);

static void exitError(const char* str)
{
	std::cerr << str << std::endl;
	exit(1);
}
