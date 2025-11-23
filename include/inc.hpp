#pragma once

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
#include <cstdlib>
#include <limits>
#include <queue>

#include <Logger.hpp>

typedef std::chrono::time_point<std::chrono::high_resolution_clock> time_point;

struct AudioComponent;
typedef std::forward_list<AudioComponent*> Components;
typedef std::vector<AudioComponent*> ComponentInput;

struct sEnvelopeADSR;
struct Master;
class Instrument;

struct MidiPlayerSettings {
	bool useKeyboardAsInput = true;
	bool splitBufferGraph = false;
};

struct AudioInfos {
	unsigned int sampleRate = {};
	unsigned int channels = {};
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
