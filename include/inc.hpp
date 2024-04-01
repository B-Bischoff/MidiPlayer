#pragma once

// [TODO] Compile from source if not installed
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <portmidi.h>
#include <RtAudio.h>

#include <iostream>
#include <chrono>
#include <chrono>
#include <cmath>
#include <assert.h>
#include <thread>
#include <cstring>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <implot.h>

#include <imgui_node_editor.h>

#define VERBOSE true
#define USE_KB_AS_MIDI_INPUT false

typedef std::chrono::time_point<std::chrono::high_resolution_clock> time_point;

struct sEnvelopeADSR;
struct Master;

enum OscType { Sine, Square, Triangle, Saw_Ana, Saw_Dig, Noise };

struct AudioData {
	unsigned int sampleRate;
	unsigned int channels;
	float bufferDuration; // In seconds
	unsigned int latency; // In frames per update

	float* buffer;
	unsigned int leftPhase, rightPhase; // only leftPhase is used in case of mono (channels = 1)
	unsigned int writeCursor;

	unsigned int targetFPS;

	int samplesToAdjust; // Used to keep read and write cursors synced in case of lag or inconsistant number of samples read over time

	RtAudio stream;

	bool syncCursors;

	std::chrono::time_point<std::chrono::high_resolution_clock> startTime;

	unsigned int getBufferSize() const
	{
		return sampleRate * bufferDuration * channels;
	}

	inline double getFramesPerUpdate() const
	{
		return (double)sampleRate / (double)targetFPS;
	}

	unsigned int getLatencyInFramesPerUpdate() const
	{
		return latency * getFramesPerUpdate() * channels;
	}

	void incrementPhases()
	{
		leftPhase = (leftPhase + channels) % (int(sampleRate * bufferDuration) * channels);
		rightPhase = (rightPhase + channels) % (int(sampleRate * bufferDuration) * channels);
	}

	void incrementWriteCursor(unsigned int increment = 1)
	{
		writeCursor = (writeCursor + increment) % ((unsigned int)(sampleRate * bufferDuration) * channels);
	}
};

// Keyboard
struct KeyData
{
	bool down; // rising edge
	bool up; // fallign edge
	bool pressed; // press state
	bool lastFramePressed;
};

struct InputManager
{
	// Midi events (PortMidi specifics)
	PmStream* midiStream;
	PmEvent buffer[32];

#define GLFW_MAX_KEY 348
	KeyData keys[GLFW_MAX_KEY];
};

void rtAudioInit(AudioData& audio);
int uploadBuffer( void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status, void *userData);
void handleInput(GLFWwindow* window, InputManager& inputManager, std::vector<sEnvelopeADSR>& envelopes, double time);
void initInput(InputManager& inputManger);
void generateAudio(AudioData& audio, Master& master, std::vector<sEnvelopeADSR>& envelopes, double& time);
int uploadBuffer(void *outputBuffer, void* inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status, void *userData);

static void exitError(const char* str)
{
	std::cerr << str << std::endl;
	exit(1);
}
