#include "inc.hpp"

void rtAudioInit(AudioData& audio)
{
	std::vector<unsigned int> deviceIds = audio.stream.getDeviceIds();
	if (deviceIds.size() < 1)
		exitError("[RTAUDIO ERROR]: No audio devices found.");

	RtAudio::StreamParameters parameters;
	parameters.deviceId = audio.stream.getDefaultOutputDevice();
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
