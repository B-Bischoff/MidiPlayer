#pragma once

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

typedef std::chrono::time_point<std::chrono::high_resolution_clock> time_point;

struct AudioComponent;
typedef std::forward_list<AudioComponent*> Components;
typedef std::vector<AudioComponent*> ComponentInput;

struct sEnvelopeADSR;
struct Master;
class Instrument;

// [TODO] Rename enums to OscSine, OscSquare, ... (?)
enum OscType { Sine, Square, Triangle, Saw_Ana, Saw_Dig, Noise };

struct MidiPlayerSettings {
	bool useKeyboardAsInput = true;
};

struct MidiInfo
{
	int keyIndex;
	int velocity; // between 0 and 255
	bool risingEdge; // Only true for the first frame, becomes false when holding key
};

struct Timer {
public:
	double duration;

	Timer(double duration) : duration(duration), _counter(0) {};
private:
	double _counter;

public:
	bool update(double deltaTime) {
		_counter -= deltaTime;
		if (_counter <= 0.0)
		{
			_counter = duration;
			return true;
		}
		return false;
	};
};
