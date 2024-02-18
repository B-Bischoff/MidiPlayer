#pragma once

#include <cmath>
#include <cstring>
#include <iostream>
#include <math.h>
#include <chrono>
#include <thread>
#include <assert.h>
#include <portmidi.h>
#include <RtAudio.h>

#define VERBOSE true
#define USE_KB_AS_MIDI_INPUT true

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

	bool test;
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

void rtAudioInit(AudioData& audio);
int uploadBuffer( void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
				double streamTime, RtAudioStreamStatus status, void *userData);

static void exitError(const char* str)
{
	std::cerr << str << std::endl;
	exit(1);
}
