#include "inc.hpp"
#include "config.hpp"

void rtAudioInit(AudioData& audio, int id = -1)
{
	std::vector<unsigned int> deviceIds = audio.stream.getDeviceIds();
	if (deviceIds.size() < 1)
	{
		Logger::log("RtAudio", Error) << "No audio device found." << std::endl;
		exit(1);
	}

	if (RT_AUDIO_DEBUG)
	{
		RtAudio& stream = audio.stream;
		Logger::log("RtAudio", Debug) << "Default audio device id: " << stream.getDefaultOutputDevice() << std::endl;

		for (const unsigned int& id : deviceIds)
		{
			const RtAudio::DeviceInfo info = stream.getDeviceInfo(id);
			Logger::log("RtAudio", Debug) << "id " << id << " Name: " << info.name << std::endl;
		}
	}

	RtAudio::StreamParameters parameters;
	parameters.deviceId = id == -1 ? audio.stream.getDefaultOutputDevice() : id;
	parameters.nChannels = audio.channels;
	parameters.firstChannel = 0;
	unsigned int sampleRate = audio.sampleRate;

#ifdef PLATFORM_WINDOWS
	// 0 is used to get the smallest frame number possible.
	// It seems to be a valid frame size on Windows (?)
	// Causing the uploadBuffer function to be called with nBufferFrames = 0
	// Which doesn't output anything ...
	unsigned int bufferFrames = sampleRate / audio.targetFPS;
#else
	unsigned int bufferFrames = 0;
#endif

	if (audio.stream.openStream(&parameters, NULL, RTAUDIO_FLOAT64, sampleRate, &bufferFrames, &uploadBuffer, &audio))
	{
		Logger::log("RtAudio", Error) << "Failed to open stream." << std::endl;
		exit(1);
	}

	if (audio.stream.startStream())
	{
		Logger::log("RtAudio", Error) << "Failed to start stream." << std::endl;
		exit(1);
	}
}
