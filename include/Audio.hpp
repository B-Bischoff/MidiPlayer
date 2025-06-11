#pragma once

#include <memory>
#include <cstring>
#include <assert.h>
#include <algorithm>
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
	~Audio();

	void update(std::vector<Instrument>& instruments, std::vector<MidiInfo>& keyPressed);

	bool mute = false;

	unsigned int getBufferSize() const;
	unsigned int getLatency() const;
	unsigned int getLatencyInSamplesPerUpdate() const;
	unsigned int getChannels() const;
	double getSamplesPerUpdate() const;
	unsigned int getSampleRate() const;
	unsigned int getWriteCursorPos() const;
	const float* getBuffer() const;
	unsigned int getTargetFPS() const;
	// cursor 0 == left phase, cursor 1 == right phase
	unsigned int getReadCursorPos(const unsigned int& cusor = 0) const;

	bool setLatency(unsigned int bufferFrameOffset);
	bool setSampleRate(unsigned int sampleRate);
	bool setChannelNumber(unsigned int channelNumber);
	bool setAudioDevice(unsigned int deviceId);

	std::vector<unsigned int> getDeviceIds();
	RtAudio::DeviceInfo getDeviceInfo(unsigned int id);
	const RtAudio::DeviceInfo& getUsedDeviceInfo() const;

private:
	// ----------------- AUDIO SETTINGS -----------------
	// In Hertz. Usually 44100 or 48000
	unsigned int _sampleRate;

	// 1 for mono, 2 for stereo
	unsigned int _channels;

	// In seconds
	unsigned int _bufferDuration;

	// Buffer frame offset between the read and write cursor (buffer frame value is defined by rtAudio).
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
	RtAudio::DeviceInfo _deviceInfo; // Informations about the used audio device

	// Internal audio time used by audio components.
	// This time is manually incremented in the update method.
	double _time;
	// -------------------------------------------------

	void initBuffer();
	bool initOutputDevice(unsigned int deviceId);

	// [TODO] Make an entity used as an intermediate between sound generation/mixing and sound management (stream open, volume, runtime reconfiguration, ...)
	static int uploadBuffer(void *outputBuffer, void* inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status, void *userData);
	void incrementPhases();
	void incrementWriteCursor();
	void copyBufferData(double* data, unsigned int sampleNumber, bool mute = false);

	void stopAndCloseStreamIfExist();
};
