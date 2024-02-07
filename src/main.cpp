#include <cmath>
#include <cstring>
#include <iostream>
#include <math.h>
#include <chrono>
#include <thread>
#include <assert.h>
#include <portmidi.h>

#if __has_include("RtAudio.h")
#include <RtAudio.h>
#else
#include <rtaudio/RtAudio.h>
#endif

#define VERBOSE false

void exitError(const std::string& error)
{
	std::cerr << error << std::endl;
	exit(1);
}

struct AudioData {
	unsigned int sampleRate;
	unsigned int channels;
	float bufferDuration; // In seconds

	float* buffer;
	unsigned int leftPhase, rightPhase;
	unsigned int writeCursor;

	unsigned int targetFPS;

	RtAudio stream;

	unsigned int getBufferSize() const
	{
		return bufferDuration * sampleRate * channels;
	}

	void incrementPhases()
	{
		leftPhase = (leftPhase + 2) % (sampleRate * channels);
		rightPhase = (rightPhase + 2) % (sampleRate * channels);
	}
};

void exitError(const char* str)
{
	std::cerr << str << std::endl;
	exit(1);
}

double freqToAngularVelocity(float hertz)
{
	return hertz * 2.0 * M_PI;
}

double ocs(float hertz, float time)
{
	double t = sin(freqToAngularVelocity(hertz) * time);
	return t;
}

void portmidi_sandbox()
{
	Pm_Initialize();

	int numDevices = Pm_CountDevices();
	if (numDevices <= 0)
	{
		std::cerr << "No MIDI devices found." << std::endl;
		exit(1);
	}

	for (int i = 0; i < numDevices; i++)
	{
		const PmDeviceInfo* info = Pm_GetDeviceInfo(i);
		assert(info);
		std::cout << i << " " << info->name << std::endl;
		std::cout << "input/output: " << info->input << " " << info->output << std::endl;
		std::cout << "open/virtual: " << info->opened << " " << info->is_virtual << std::endl;
		std::cout << std::endl;
	}

	PmStream* midiStream;
	Pm_OpenInput(&midiStream, 3, NULL, 512, NULL, NULL);

	PmEvent buffer[32];

	while(1)
	{
		int numEvents = Pm_Read(midiStream, buffer, 32);

		for (int i = 0; i < numEvents; i++)
		{
			PmEvent& event = buffer[i];

			// Extract MIDI status and data bytes
			PmMessage message = event.message;
			int status = Pm_MessageStatus(message);
			int data1 = Pm_MessageData1(message);
			int data2 = Pm_MessageData2(message);

			// Print MIDI event information
			std::cout << " state " << status
					<< " key " << (int)data1
					<< " vel " << (int)data2 << std::endl;
		}
		//Pa_Sleep(1);
	}

	Pm_Close(midiStream);
	Pm_Terminate();
}

int saw( void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
				double streamTime, RtAudioStreamStatus status, void *userData )
{
	double *buffer = (double *) outputBuffer;
	AudioData& audio = *(AudioData*)userData;

	//std::cout << "callback time : " << streamTime << std::endl;
	if (status)
		std::cout << "Stream underflow detected!" << std::endl;

	//std::cout << nBufferFrames << " | " << rb.writeCursor << " " << rb.leftPhase << std::endl;
	for (int i = 0 ; i<nBufferFrames; i++)
	{
		double tmp;
		// When using the whole stream time (decimal part included), audio produces
		// a weird sound amplified when stream time reaches a power of 2.
		// This error is probably (not 100% sure) due to floating point precision.
		double t = std::modf(streamTime, &tmp) + (1.0f / 44100.0f * (double)(i));
		double value = ocs(440, t) * 0.5f;

		*buffer++ = audio.buffer[audio.leftPhase];
		*buffer++ = audio.buffer[audio.rightPhase];
		audio.incrementPhases();
	}
	std::cout << audio.leftPhase / 2 << " " << audio.writeCursor / 2.0f << std::endl;

	return 0;
}

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
	unsigned int bufferFrames = sampleRate / audio.targetFPS;

	if (audio.stream.openStream(&parameters, NULL, RTAUDIO_FLOAT64, sampleRate, &bufferFrames, &saw, &audio))
		exitError("[RTAUDIO ERROR]: Cannot open stream.");

	if (audio.stream.startStream())
		exitError("[RTAUDIO ERROR]: Cannot start stream.");
}

int main(void)
{
	AudioData audio = {
		.sampleRate = 44100,
		.channels = 2,
		.bufferDuration = 1,
		.buffer = nullptr,
		.leftPhase = 0, .rightPhase = 1,
		.writeCursor = 0,
		.targetFPS = 60,
	};

	const std::chrono::duration<double> targetFrameDuration(1.0f / (double)audio.targetFPS);

	audio.buffer = new float[audio.sampleRate * audio.channels];

	if (audio.buffer == nullptr)
		exitError("[ERROR]: ring buffer allocation failed.");

	memset(audio.buffer, 0, sizeof(float) * audio.getBufferSize());

	rtAudioInit(audio);

	//portmidi_sandbox();

	//std::thread midiThread(portmidi_sandbox);

	auto programStartTime = std::chrono::high_resolution_clock::now();

	audio.writeCursor = (44100.0f / (double)audio.targetFPS) * 35.0f;
	while (1)
	{
		auto startTime = std::chrono::high_resolution_clock::now();

		auto programElapsedTime = std::chrono::duration_cast<std::chrono::duration<double>>(startTime - programStartTime);
		//std::cout << programElapsedTime.count() << std::endl;

		for (int i = 0; i < audio.sampleRate / audio.targetFPS; i++)
		{
			double tmp;
			double t = std::modf(programElapsedTime.count(), &tmp) + (1.0f / 44100.0f * (double)(i));
			double value = ocs(440, t) * 0.5f;

			for (int j = 0; j < audio.channels; j++)
			{
				audio.buffer[audio.writeCursor] = value;
				audio.writeCursor = (audio.writeCursor + 1) % (audio.sampleRate * audio.channels);
			}
		}
		//std::cout << "wrote " << rb.writeCursor << std::endl;

		auto endTime = std::chrono::high_resolution_clock::now();
		auto deltaTime = std::chrono::duration_cast<std::chrono::duration<double>>(endTime - startTime);

		auto sleepDuration = targetFrameDuration - deltaTime;

		if (sleepDuration > std::chrono::duration<double>(0.0))
		{
			std::this_thread::sleep_for(sleepDuration * 0.9f);
			endTime = std::chrono::high_resolution_clock::now();
			while (endTime - startTime < targetFrameDuration)
				endTime = std::chrono::high_resolution_clock::now();
		}
		else
			std::cerr << "[WARNING] : update took longer than expected" << std::endl;
	}
}
