#include "Audio.hpp"

Audio::Audio(unsigned int sampleRate, unsigned int channels, unsigned int bufferDuration, unsigned int latency)
	: _sampleRate(sampleRate), _channels(channels), _bufferDuration(bufferDuration), _latency(latency),
	_targetFPS(60), _buffer(nullptr), _leftPhase(0), _rightPhase(1), _writeCursor(0)
{
	initBuffer();
	_startTime = std::chrono::high_resolution_clock::now();
	initOutputDevice();

	std::this_thread::sleep_for(std::chrono::milliseconds(100)); // let rtaudio get more stable [TODO] check if that is necessary
	_writeCursor = (_leftPhase + getLatencyInFramesPerUpdate()) % getBufferSize();

}

void Audio::initBuffer()
{
	_buffer = std::make_unique<float[]>(getBufferSize());
	if (_buffer == nullptr)
	{
		Logger::log("Audio", Error) << "Failed to allocate buffer (size: " << std::to_string(getBufferSize()) << ")" << std::endl;
		exit(1);
	}

	std::memset((void*)_buffer.get(), 0, sizeof(float) * getBufferSize());
}

void Audio::initOutputDevice()
{
	std::vector<unsigned int> deviceIds = _stream.getDeviceIds();
	if (deviceIds.size() < 1)
	{
		Logger::log("RtAudio", Error) << "No audio device found." << std::endl;
		exit(1);
	}

	if (RT_AUDIO_DEBUG)
	{
		RtAudio& stream = _stream;
		Logger::log("RtAudio", Debug) << "Default audio device id: " << stream.getDefaultOutputDevice() << std::endl;

		for (const unsigned int& id : deviceIds)
		{
			const RtAudio::DeviceInfo info = stream.getDeviceInfo(id);
			Logger::log("RtAudio", Debug) << "id " << id << " Name: " << info.name << std::endl;
		}
	}

	RtAudio::StreamParameters parameters;
	parameters.deviceId = _stream.getDefaultOutputDevice();
	parameters.nChannels = _channels;
	parameters.firstChannel = 0;
	const unsigned int sampleRate = _sampleRate;

#ifdef PLATFORM_WINDOWS
	// 0 is used to get the smallest frame number possible.
	// It seems to be a valid frame size on Windows (?)
	// Causing the uploadBuffer function to be called with nBufferFrames = 0
	// Which doesn't output anything ...
	unsigned int bufferFrames = sampleRate / audio.targetFPS;
#else
	unsigned int bufferFrames = 0;
#endif

	if (_stream.openStream(&parameters, NULL, RTAUDIO_FLOAT64, sampleRate, &bufferFrames, &uploadBuffer, this))
	{
		Logger::log("RtAudio", Error) << "Failed to open stream." << std::endl;
		exit(1);
	}

	if (_stream.startStream())
	{
		Logger::log("RtAudio", Error) << "Failed to start stream." << std::endl;
		exit(1);
	}
}

void Audio::update(std::vector<Instrument>& instruments, std::vector<MidiInfo>& keyPressed, double& time)
{
	static int TEST = 0;
	const double fractionalPart = getFramesPerUpdate() - static_cast<int>(getFramesPerUpdate());
	int complementaryFrame = 0;
	bool writeOneMoreFrame = false;
	if (fractionalPart != 0.0)
	{
		complementaryFrame = std::ceil(1.0 / fractionalPart);
		writeOneMoreFrame = TEST % complementaryFrame == 0;
	}
	TEST++;
	//std::cout << writeOneMoreFrame << std::endl;
	//for (int i = 0; i < audio.sampleRate / audio.targetFPS + (int)writeOneMoreFrame; i++)

	for (int i = 0; i < static_cast<int>(getFramesPerUpdate()) + writeOneMoreFrame + _samplesToAdjust; i++)
	{
		double value = 0.0;

		value = 0.0;
		for (Instrument& instrument : instruments)
			value += instrument.process(keyPressed) * 1.0;

		//if (writeOneMoreFrame && i == audio.sampleRate/audio.targetFPS)
		//	value = 0;
		//std::cout << "TIME : " << time << " " << value << " " << std::endl;
		//t += (double)audio.sampleRate / (double)audio.targetFPS;
		time += 1.0 / static_cast<double>(_sampleRate);
		AudioComponent::time = time;

		for (int j = 0; j < _channels; j++)
		{
			_buffer[_writeCursor] = value;
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
	double *buffer = (double*)outputBuffer;

	//std::cout << "callback time : " << streamTime << std::endl;
	if (status) Logger::log("Audio", Warning) << "Stream underflow detected." << std::endl;

	audio->copyBufferData(buffer, nBufferFrames);

	if (audio->_syncCursors)
	{
		audio->_syncCursors = false;
		int cursorsDelta = audio->_writeCursor - audio->_leftPhase;
		if (cursorsDelta < 0)
			cursorsDelta += audio->getBufferSize();

		audio->_samplesToAdjust = audio->getLatencyInFramesPerUpdate() - cursorsDelta;
	}

	return 0;
}

void Audio::copyBufferData(double* data, unsigned int sampleNumber)
{
	for (int sample = 0; sample < sampleNumber; sample++)
	{
		*data++ = _buffer[_leftPhase];
		if (_channels == 2)
			*data++ = _buffer[_rightPhase];
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

double Audio::getFramesPerUpdate() const
{
	return static_cast<double>(_sampleRate) / static_cast<double>(_targetFPS);
}

unsigned int Audio::getLatency() const
{
	return _latency;
}

unsigned int Audio::getLatencyInFramesPerUpdate() const
{
	return _latency * getFramesPerUpdate() * _channels;
}

unsigned int Audio::getChannels() const
{
	return _channels;
}

unsigned int Audio::getSampleRate() const
{
	return _sampleRate;
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

time_point Audio::getStartTime() const
{
	return _startTime;
}
