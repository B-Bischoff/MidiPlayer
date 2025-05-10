#pragma once

#include <memory>
#include <cstring>
#include <assert.h>
#include <RtAudio.h>

#include "AudioBackend/Instrument.hpp"
#include "inc.hpp"

#include "Logger.hpp"
#include "config.hpp"

class Audio {
public:
	Audio(
		unsigned int sampleRate = 44100,
		unsigned int channels = 2,
		unsigned int bufferDuration = 1,
		unsigned int latency = 3
	);

	void update(std::vector<Instrument>& instruments, std::vector<MidiInfo>& keyPressed, double& time);

	unsigned int getBufferSize() const;
	unsigned int getLatency() const;
	unsigned int getLatencyInFramesPerUpdate() const;
	unsigned int getChannels() const;
	double getFramesPerUpdate() const;
	unsigned int getSampleRate() const;
	unsigned int getWriteCursorPos() const;
	const float* getBuffer() const;
	time_point getStartTime() const;
	unsigned int getTargetFPS() const;
	// cursor 0 == left phase, cursor 1 == right phase
	unsigned int getReadCursorPos(const unsigned int& cusor = 0) const;

private:
	// ----------------- AUDIO SETTINGS -----------------
	// In Hertz. Usually 44100 or 48000
	unsigned int _sampleRate;

	// 1 for mono, 2 for stereo
	unsigned int _channels;

	// In seconds
	unsigned int _bufferDuration;

	// Buffer frame offset between the read and write cursor.
	// On slow computer, a too small value may cause the read cusor to overtake the write cursor.
	unsigned int _latency;

	unsigned int _targetFPS;

	// ----------------- INTERNAL DATA -----------------
	std::unique_ptr<float[]> _buffer;
	unsigned int _leftPhase, _rightPhase; // Read cursors
	unsigned int _writeCursor;
	bool _syncCursors;
	int _samplesToAdjust; // Used to keep read and write cursors synced in case of lag or inconsistant number of samples read over time
	RtAudio _stream;

	time_point _startTime;

	void initBuffer();
	void initOutputDevice();

	// [TODO] Make an entity used as an intermediate between sound generation/mixing and sound management (stream open, volume, runtime reconfiguration, ...)
	static int uploadBuffer(void *outputBuffer, void* inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status, void *userData);
	void incrementPhases();
	void incrementWriteCursor();
	void copyBufferData(double* data, unsigned int sampleNumber);
};
