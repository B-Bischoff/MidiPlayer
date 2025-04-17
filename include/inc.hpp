#pragma once

#define GLEW_STATIC
#include "minimp3.h"
#include "minimp3_ex.h"

// [TODO] Compile from source if not installed
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <forward_list>
#include <portmidi.h>
#include <RtAudio.h>

#include <iostream>
#include <chrono>
#include <chrono>
#include <cmath>
#include <assert.h>
#include <thread>
#include <cstring>
#include <limits>
#include <queue>

#include <implot.h>
#include <imgui_node_editor.h>

#include <Logger.hpp>

// [TODO] This should not be in UI
#include <UI/Message.hpp>

typedef unsigned int id;
typedef std::chrono::time_point<std::chrono::high_resolution_clock> time_point;

struct AudioComponent;
typedef std::forward_list<AudioComponent*> Components;
typedef std::vector<AudioComponent*> ComponentInput;

struct sEnvelopeADSR;
struct Master;
class Instrument;

// [TODO] Rename enums to OscSine, OscSquare, ... (?)
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

struct MidiPlayerSettings {
	bool useKeyboardAsInput = true;
	bool freezeAudioGeneration = false;
};

struct MidiInfo
{
	int keyIndex;
	int velocity; // between 0 and 255
	bool risingEdge; // Only true for the first frame, becomes false when holding key
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
	PmStream* midiStream = nullptr;
	PmEvent buffer[32];

	// Keyboard data
	KeyData keys[GLFW_KEY_LAST] = {};
	unsigned int octave = 4;
	static constexpr unsigned int maxOctave = 8;

	// Mouse
	static constexpr int GLFW_MAX_MOUSE_BTN = 8;
	KeyData mouseButtons[GLFW_MAX_MOUSE_BTN];
	// [TODO] use glm?
	ImVec2 cursorPos;
	ImVec2 cursorDir;
	// [TODO] add scroll
};

void rtAudioInit(AudioData& audio, int id);
int uploadBuffer(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status, void *userData);
void updateKeysState(GLFWwindow* window, const MidiPlayerSettings& settings, InputManager& inputManager, std::vector<MidiInfo>& keyPressed, double time);
void glfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void initInput(InputManager& inputManger);
void generateAudio(AudioData& audio, std::vector<Instrument>& instruments, std::vector<MidiInfo>& keyPressed, double& time);
int uploadBuffer(void *outputBuffer, void* inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status, void *userData);
void createKeysEvents(InputManager& inputManager, std::queue<Message>& messageQueue);
