#include "Audio.hpp"

Audio::Audio(unsigned int sampleRate, unsigned int channels, unsigned int bufferDuration, unsigned int latency)
	: _sampleRate(sampleRate), _channels(channels), _bufferDuration(bufferDuration), _latency(latency),
	_targetFPS(60), _buffer(nullptr), _leftPhase(0), _rightPhase(1), _writeCursor(0), _syncCursors(false),
	_samplesToAdjust(0), _time(0.0)
{
	initBuffer();
	initOutputDevice(0); // Open system default audio device

	std::this_thread::sleep_for(std::chrono::milliseconds(100)); // let rtaudio get more stable

	// Read cursors might have already moved, so make write cursor point ahead of it.
	_writeCursor = (_leftPhase + getLatencyInSamplesPerUpdate()) % getBufferSize();
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

void Audio::initBuffer()
{
	// Stop registered callback from reading buffer while it's being (re)allocated
	if (_stream.isStreamRunning())
		_stream.abortStream();

	_buffer = std::make_unique<float[]>(getBufferSize());
	if (_buffer == nullptr)
	{
		Logger::log("Audio", Error) << "Failed to allocate buffer (size: " << std::to_string(getBufferSize()) << ")" << std::endl;
		exit(1);
	}

	std::memset((void*)_buffer.get(), 0, sizeof(float) * getBufferSize());

	_leftPhase = 0;
	_rightPhase = 1;
	_writeCursor = (_leftPhase + getLatencyInSamplesPerUpdate()) % getBufferSize();
}

bool Audio::initOutputDevice(unsigned int deviceId)
{
	stopAndCloseStreamIfExist();

	std::vector<unsigned int> deviceIds = _stream.getDeviceIds();
	if (deviceIds.size() < 1)
	{
		Logger::log("RtAudio", Error) << "No audio device found." << std::endl;
		return true;
	}

	RtAudio::StreamParameters parameters;
	parameters.deviceId = deviceId == 0 ? _stream.getDefaultOutputDevice() : deviceId;
	parameters.nChannels = _channels;
	parameters.firstChannel = 0; // left ear in stereo
	const unsigned int streamSampleRate = _sampleRate;
	unsigned int bufferFrames = streamSampleRate / _targetFPS;

	if (_stream.openStream(&parameters, NULL, RTAUDIO_FLOAT32, streamSampleRate, &bufferFrames, &uploadBuffer, this) != RTAUDIO_NO_ERROR)
	{
		_deviceInfo = {};
		Logger::log("RtAudio", Error) << "Failed to open stream." << std::endl;
		return true;
	}

	if (_stream.startStream() != RTAUDIO_NO_ERROR)
	{
		Logger::log("RtAudio", Error) << "Failed to start stream." << std::endl;
		return true;
	}

	Logger::log("Audio", Info) << "Successfully opened audio stream with the following properties:" << std::endl;
	Logger::log("Audio", Info)  << "Sample rate: " << _stream.getStreamSampleRate() << "Hz" << std::endl;
	Logger::log("Audio", Info)  << "Channel number: " << _channels << std::endl;
	Logger::log("Audio", Info)  << "Buffer duration: " << _bufferDuration << " second(s)" << std::endl;

	_sampleRate = _stream.getStreamSampleRate();
	_deviceInfo = _stream.getDeviceInfo(parameters.deviceId);

	return false;
}

void Audio::update(std::vector<Instrument>& instruments, std::vector<MidiInfo>& keyPressed)
{
	std::chrono::duration<double> frameDuration(1.0 / static_cast<double>(_sampleRate));

	const int samplesToGenerate = static_cast<int>(getSamplesPerUpdate()) + _samplesToAdjust;

	for (int i = 0; i < samplesToGenerate; i++)
	{
		double value = 0.0;

		const AudioInfos audioInfos = {
			.sampleRate = _sampleRate,
			.channels = _channels
		};

		for (Instrument& instrument : instruments)
			value += instrument.process(audioInfos, keyPressed) * 1.0;

		_time += 1.0 / static_cast<double>(_sampleRate);
		AudioComponent::time = _time;

		for (int j = 0; j < _channels; j++)
		{
			_buffer[_writeCursor] = std::clamp(value, -1.0, 1.0);
			incrementWriteCursor();
		}
	}

	//assert(audio.syncCursors == false && "Audio callback did not reset syncCursors");
	_syncCursors = true;
}

int Audio::uploadBuffer(void *outputBuffer, void* inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status, void *userData)
{
	Audio* audio = static_cast<Audio*>(userData);
	assert(audio);
	float *buffer = (float*)outputBuffer;

	//std::cout << "callback time : " << streamTime << std::endl;
	if (status) Logger::log("Audio", Warning) << "Stream underflow detected." << std::endl;

	audio->copyBufferData(buffer, nBufferFrames, audio->mute);

	if (audio->_syncCursors)
	{
		audio->_syncCursors = false;
		int cursorsDelta = audio->_writeCursor - audio->_leftPhase;
		if (cursorsDelta < 0)
			cursorsDelta += audio->getBufferSize();

		audio->_samplesToAdjust = audio->getLatencyInSamplesPerUpdate() - cursorsDelta;
	}

	return 0;
}

void Audio::copyBufferData(float* data, unsigned int sampleNumber, bool mute)
{
	for (int sample = 0; sample < sampleNumber; sample++)
	{
		*data++ = mute ? 0 : _buffer[_leftPhase];
		if (_channels == 2)
			*data++ = mute ? 0 : _buffer[_rightPhase];
		incrementPhases();
	}
}

unsigned int Audio::getBufferSize() const
{
	return _sampleRate * _bufferDuration * _channels;
}

void Audio::incrementPhases()
{
	_leftPhase = (_leftPhase + _channels) % getBufferSize();
	_rightPhase = (_rightPhase + _channels) % getBufferSize();
}

void Audio::incrementWriteCursor()
{
	_writeCursor = (_writeCursor + 1) % getBufferSize();
}

double Audio::getSamplesPerUpdate() const
{
	return static_cast<double>(_sampleRate) / static_cast<double>(_targetFPS);
}

unsigned int Audio::getLatency() const
{
	return _latency;
}

bool Audio::setLatency(unsigned int bufferFrameOffset)
{
	if (bufferFrameOffset == _latency)
		return false;

	const double durationInSeconds = bufferFrameOffset * getSamplesPerUpdate() / _sampleRate;

	// Arbitrary limits
	if (bufferFrameOffset > 30 || bufferFrameOffset == 0)
	{
		Logger::log("Audio", Warning) << "Invalid latency: " << bufferFrameOffset << " (" << durationInSeconds << " seconds)" << std::endl;
		return true;
	}

	Logger::log("Audio", Info) << "New latency: " << bufferFrameOffset << " buffer frame (" << durationInSeconds << " seconds)" << std::endl;
	_latency = bufferFrameOffset;

	return false;
}

unsigned int Audio::getLatencyInSamplesPerUpdate() const
{
	return _latency * getSamplesPerUpdate() * _channels;
}

bool Audio::setChannelNumber(unsigned int channelNumber)
{
	if (channelNumber == _channels)
		return false;

	_channels = channelNumber;
	initBuffer();
	return initOutputDevice(_deviceInfo.ID);
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
	initBuffer();
	return initOutputDevice(_deviceInfo.ID);
}

unsigned int Audio::getWriteCursorPos() const
{
	return _writeCursor;
}

unsigned int Audio::getReadCursorPos(const unsigned int& cursor) const
{
	if (cursor > 1)
	{
		Logger::log("Audio", Error) << "cursor should be 0 (left phase) or 1 (right phase)" << std::endl;
		exit(1);
	}
	return cursor == 0 ? _leftPhase : _rightPhase;
}

const float* Audio::getBuffer() const
{
	return _buffer.get();
}

unsigned int Audio::getTargetFPS() const
{
	return _targetFPS;
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
