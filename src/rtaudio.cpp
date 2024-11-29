#include "inc.hpp"
#include "config.hpp"

void rtAudioInit(AudioData& audio, int id = -1)
{
	std::vector<unsigned int> deviceIds = audio.stream.getDeviceIds();
	if (deviceIds.size() < 1)
		exitError("[RTAUDIO ERROR]: No audio devices found.");

	if (RT_AUDIO_DEBUG)
	{
		RtAudio& stream = audio.stream;
		std::cout << "Default audio device id: " << stream.getDefaultOutputDevice() << std::endl;

		for (const unsigned int& id : deviceIds)
		{
			const RtAudio::DeviceInfo info = stream.getDeviceInfo(id);
			std::cout << "id " << id << " Name: " << info.name << std::endl;
		}
	}

	RtAudio::StreamParameters parameters;
	parameters.deviceId = id == -1 ? audio.stream.getDefaultOutputDevice() : id;
	parameters.nChannels = audio.channels;
	parameters.firstChannel = 0;
	unsigned int sampleRate = audio.sampleRate;
	//unsigned int bufferFrames = sampleRate / audio.targetFPS;
	unsigned int bufferFrames = 0;
	//unsigned int bufferFrames = 735;

	if (audio.stream.openStream(&parameters, NULL, RTAUDIO_FLOAT64, sampleRate, &bufferFrames, &uploadBuffer, &audio))
		exitError("[RTAUDIO ERROR]: Cannot open stream.");

	if (audio.stream.startStream())
		exitError("[RTAUDIO ERROR]: Cannot start stream.");
}
