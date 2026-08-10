#pragma once

#include <memory>
#include <cstring>
#include <assert.h>
#include <algorithm>
#include <atomic>
#include <RtAudio.h>

#include "AudioBackend/Instrument.hpp"
#include "inc.hpp"

#include "Logger.hpp"
#include "config.hpp"

#define DEFAULT_AUDIO_DEVICE_ID 0
#define DEFAULT_BUFFER_SIZE_FRAMES 1024

class Audio {
public:
	Audio(unsigned int sampleRate = 44100, unsigned int channels = 2, unsigned int requestedBufferSizeFrames = DEFAULT_BUFFER_SIZE_FRAMES);
	~Audio();

	void update(std::vector<Instrument>& instruments, std::vector<MidiInfo>& keyPressed);

	bool mute = false;

	unsigned int getChannels() const;
	unsigned int getSampleRate() const;
	unsigned int getWriteCursorPos() const;
	unsigned int getReadCursorPos() const;
	const std::vector<float>& getBuffer() const;

	bool setSampleRate(unsigned int sampleRate);
	bool setChannelNumber(unsigned int channelNumber);
	bool setAudioDevice(unsigned int deviceId);

	std::vector<unsigned int> getDeviceIds();
	RtAudio::DeviceInfo getDeviceInfo(unsigned int id);
	const RtAudio::DeviceInfo& getUsedDeviceInfo() const;

private:
	bool initOutputDevice(unsigned int requestedBufferSizeFrames, unsigned int deviceId = DEFAULT_AUDIO_DEVICE_ID);

	// [TODO] Make an entity used as an intermediate between sound generation/mixing and sound management (stream open, volume, runtime reconfiguration, ...)
	static int uploadBuffer(void *outputBuffer, void* inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status, void *userData);
	void copyBufferData(float* data, unsigned int sampleNumber, bool mute = false);

	void stopAndCloseStreamIfExist();

	RtAudio _stream;
	RtAudio::DeviceInfo _deviceInfo; // Informations about the used audio device

	// Thread-safe Ring Buffer
	std::vector<float> _ringBuffer;
	unsigned int _bufferCapacitySamples;

	// Only the main thread modifies this
	unsigned int _writeCursor;

	// Only the audio callback modifies this
	unsigned int _readCursor;

	// Shared between threads to track how much unread audio exists
	std::atomic<int> _availableSamples;

	// In Hertz. Usually 44100 or 48000
	unsigned int _sampleRate;

	// 1 for mono, 2 for stereo
	unsigned int _channels;

	// Internal audio time used by audio components.
	// This time is manually incremented in the update method.
	double _time;
};
