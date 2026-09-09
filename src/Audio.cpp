#include "Audio.hpp"

Audio::Audio(unsigned int sampleRate, unsigned int channels, unsigned int requestedBufferSizeFrames)
	: _writeCursor(0), _readCursor(0), _availableSamples(0), _sampleRate(sampleRate), _channels(channels), _time(0.0)
{
	initOutputDevice(requestedBufferSizeFrames); // Open system default audio device
}

Audio::~Audio()
{
	stopAndCloseStreamIfExist();
}

void Audio::stopAndCloseStreamIfExist()
{
	if (_stream.isStreamRunning())
		_stream.stopStream();
	if (_stream.isStreamOpen())
		_stream.closeStream();
}

bool Audio::initOutputDevice(unsigned int requestedBufferSizeFrames, unsigned int deviceId)
{
	stopAndCloseStreamIfExist();

	if (_stream.getDeviceCount() < 1)
	{
		Logger::log("RtAudio", Error) << "No audio device found." << std::endl;
		return true;
	}

	RtAudio::StreamParameters parameters;
	parameters.deviceId = deviceId == 0 ? _stream.getDefaultOutputDevice() : deviceId;
	parameters.nChannels = _channels;
	parameters.firstChannel = 0; // left ear in stereo

	// RtAudio will overwrite this variable with the hardware's actual buffer size!
	unsigned int actualBufferFrames = requestedBufferSizeFrames;

	if (_stream.openStream(&parameters, NULL, RTAUDIO_FLOAT32, _sampleRate,
				&actualBufferFrames, &uploadBuffer, this) != RTAUDIO_NO_ERROR)
	{
		_deviceInfo = {};
		Logger::log("RtAudio", Error) << "Failed to open stream." << std::endl;
		return true;
	}

	// Allocate the ring buffer. We make it 4x the callback size to easily absorb
	// any OS scheduling jitter from the main thread.
	_bufferCapacitySamples = actualBufferFrames * _channels * 4;
	_ringBuffer.resize(_bufferCapacitySamples, 0.0f);

	_writeCursor = 0;
	_readCursor = 0;
	_availableSamples.store(0, std::memory_order_release);

	if (_stream.startStream() != RTAUDIO_NO_ERROR)
	{
		Logger::log("RtAudio", Error) << "Failed to start stream." << std::endl;
		return true;
	}

	// Update internal state based on what the hardware actually gave us
	_sampleRate = _stream.getStreamSampleRate();
	_deviceInfo = _stream.getDeviceInfo(parameters.deviceId);

	Logger::log("Audio", Info) << "Opened audio stream: " << _sampleRate << "Hz, "
								<< _channels << " channels. Hardware buffer: "
								<< actualBufferFrames << " frames.\n";

	return false;
}

// ----------------------------------------------------------------------------
// PRODUCER: Main Thread
// ----------------------------------------------------------------------------
void Audio::update(std::vector<Instrument>& instruments, std::vector<MidiInfo>& keyPressed)
{
	// Check available space in the buffer
	int currentAvailable = _availableSamples.load(std::memory_order_acquire);
	int freeSpace = _bufferCapacitySamples - currentAvailable;

	// Only generate full frames (pairs of Left/Right samples)
	int framesToGenerate = freeSpace / _channels;

	AudioInfos baseAudioInfos = { _sampleRate, _channels, 0 };

	int samplesGenerated = 0;


	for (int i = 0; i < framesToGenerate; i++)
	{
		for (unsigned int channel = 0; channel < _channels; channel++)
		{
			baseAudioInfos.currentChannel = channel;

			double value = 0.0;
			for (Instrument& instrument : instruments)
				value += instrument.process(baseAudioInfos, keyPressed);

			// Write to buffer and advance the write cursor
			_ringBuffer[_writeCursor] = static_cast<float>(std::clamp(value, -1.0, 1.0));
			_writeCursor = (_writeCursor + 1) % _bufferCapacitySamples;
			samplesGenerated++;
		}

		_time += 1.0 / static_cast<double>(_sampleRate);
		AudioComponent::time = _time;
	}

	// Safely notify the audio callback that new samples are ready
	if (samplesGenerated > 0)
	{
		_availableSamples.fetch_add(samplesGenerated, std::memory_order_release);
		// Logger::log("Audio", Debug) << "Generated " << samplesGenerated << " samples, "
		// 							<< _availableSamples.load(std::memory_order_acquire)
		// 							<< " samples available in buffer." << std::endl;;
	}
}

// ----------------------------------------------------------------------------
// CONSUMER: Hardware Audio Thread
// ----------------------------------------------------------------------------
int Audio::uploadBuffer(void* outputBuffer, void* /*inputBuffer*/, unsigned int nBufferFrames,
						double /*streamTime*/, RtAudioStreamStatus status, void* userData)
{
	Audio* audio = static_cast<Audio*>(userData);
	float *out = static_cast<float*>(outputBuffer);

	if (status)
		Logger::log("Audio", Warning) << "Stream underflow detected." << std::endl;

	int samplesRequested = nBufferFrames * audio->_channels;
	int availableSamples = audio->_availableSamples.load(std::memory_order_acquire);

	// If the main thread hasn't generated enough audio, output silence (underrun)
	if (availableSamples < samplesRequested)
	{
		std::fill(out, out + samplesRequested, 0.0f);
		Logger::log("Audio", Warning) << "Buffer underrun! Main thread is too slow.\n";
		return 0;
	}

	// Read requested samples into the hardware buffer
	for (int i = 0; i < samplesRequested; i++)
	{
		out[i] = audio->_ringBuffer[audio->_readCursor];
		audio->_readCursor = (audio->_readCursor + 1) % audio->_bufferCapacitySamples;
	}

	// Safely notify the main thread that space has freed up
	audio->_availableSamples.fetch_sub(samplesRequested, std::memory_order_release);

	return 0;
}

bool Audio::setChannelNumber(unsigned int channelNumber)
{
	if (channelNumber == _channels)
		return false;

	_channels = channelNumber;
	return initOutputDevice(DEFAULT_BUFFER_SIZE_FRAMES, _deviceInfo.ID);
}

unsigned int Audio::getChannels() const
{
	return _channels;
}

unsigned int Audio::getSampleRate() const
{
	return _sampleRate;
}

bool Audio::setSampleRate(unsigned int sampleRate)
{
	if (sampleRate == _sampleRate)
		return false;

	_sampleRate = sampleRate;
	return initOutputDevice(DEFAULT_BUFFER_SIZE_FRAMES, _deviceInfo.ID);
}

unsigned int Audio::getWriteCursorPos() const
{
	return _writeCursor;
}

unsigned int Audio::getReadCursorPos() const
{
	return _readCursor;
}

const std::vector<float>& Audio::getBuffer() const
{
	return _ringBuffer;
}

std::vector<unsigned int> Audio::getDeviceIds()
{
	return _stream.getDeviceIds();
}

RtAudio::DeviceInfo Audio::getDeviceInfo(unsigned int id)
{
	return _stream.getDeviceInfo(id);
}

bool Audio::setAudioDevice(unsigned int deviceId)
{
	return initOutputDevice(deviceId);
}

const RtAudio::DeviceInfo& Audio::getUsedDeviceInfo() const
{
	return _deviceInfo;
}
