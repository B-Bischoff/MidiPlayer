#include <cmath>
#include <cstring>
#include <iostream>
#include <math.h>
#include <thread>
#include <assert.h>

#include <portaudio.h>
#include <portmidi.h>

#define VERBOSE false

#define AUDIO_SIZE 44100

void exitError(const std::string& error)
{
	std::cerr << error << std::endl;
	exit(1);
}

struct RingBuffer {
	float* buffer; // To make true stereo, this buffer should be twice its size
	PaStream* stream;
	unsigned int leftPhase, rightPhase;
};

struct AudioProperties {
	unsigned int sampleRate;
	unsigned int channels;
	PaSampleFormat sampleFormat; // typedef to unsigned long
};

struct AudioData {
	RingBuffer ringBuffer;
	AudioProperties properties;
};

void exitError(const char* str)
{
	std::cerr << str << std::endl;
	exit(1);
}

float freqToAngularVelocity(float hertz)
{
	return hertz * 2.0f * M_PI;
}

float ocs(float hertz, float time)
{
	float t = 10000 * sinf(freqToAngularVelocity(hertz) * time);
	return t;
}

void printPaDeviceInfo(const PaDeviceInfo* deviceInfo)
{
	assert(deviceInfo != nullptr && "Invalid deviceInfo");

	std::cout << "name : " << deviceInfo->name << std::endl;
	std::cout << "max input / output channels : " << deviceInfo->maxInputChannels << " " << deviceInfo->maxOutputChannels << std::endl;
	std::cout << "max default low input / ouput latency : " << deviceInfo->defaultLowInputLatency << " " << deviceInfo->defaultLowOutputLatency << std::endl;
	std::cout << "default sample rate : " << deviceInfo->defaultSampleRate << std::endl;
	std::cout << std::endl;
}

static int paCallback(const void* inputBuffer, void* outputBuffer,
		unsigned long framesPerBuffer,
		const PaStreamCallbackTimeInfo* timeInfo,
		PaStreamCallbackFlags statusFlags,
		void* data)
{
	AudioData& audioData = *(AudioData*)data;
	RingBuffer& ringBuffer = audioData.ringBuffer;
	float* out = (float*)outputBuffer;

	float* t = ringBuffer.buffer;

	for(int i = 0; i < audioData.properties.sampleRate; i++)
	{
		ringBuffer.buffer[i] = 0.00005f * ocs(440, (float)i / 44100.0f);
	}

	//std::cout << "frames per buffer : " << framesPerBuffer <<  std::endl;
	//std::cout << "time elapsed: " << timeInfo->inputBufferAdcTime << std::endl;

	for (int i = 0; i < framesPerBuffer; i++)
	{
		*out++ = ringBuffer.buffer[ringBuffer.leftPhase];
		*out++ = ringBuffer.buffer[ringBuffer.rightPhase];
		ringBuffer.leftPhase = (ringBuffer.leftPhase + 1) % audioData.properties.sampleRate;
		ringBuffer.rightPhase = (ringBuffer.rightPhase + 1) % audioData.properties.sampleRate;
	}

	return paContinue;
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
		Pa_Sleep(1);
	}

	Pm_Close(midiStream);
	Pm_Terminate();
}

void portaudio_sandbox(AudioData& audioData)
{
	if (Pa_Initialize() != paNoError)
		exitError("[ERROR] Port Audio : initialization failed");

	int numDevices = Pa_GetDeviceCount();

	for (int deviceIndex = 0; deviceIndex < numDevices; deviceIndex++)
	{
		const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(deviceIndex);
#if VERBOSE
		std::cout << "Device id : " << deviceIndex << std::endl;
		printPaDeviceInfo(deviceInfo);
#endif
	}

	int defaultDevice = Pa_GetDefaultOutputDevice();
	if (defaultDevice == paNoDevice)
		exitError("[ERROR] Port Audio : no default output device found" );

	std::cout << "(default) Device id : " << defaultDevice << std::endl;
	printPaDeviceInfo(Pa_GetDeviceInfo(defaultDevice));

	PaStreamParameters outputParameters;
	outputParameters.device = defaultDevice;
	outputParameters.channelCount = audioData.properties.channels;
	outputParameters.sampleFormat = audioData.properties.sampleFormat;
	outputParameters.suggestedLatency = Pa_GetDeviceInfo(defaultDevice)->defaultLowOutputLatency;
	outputParameters.hostApiSpecificStreamInfo = nullptr;

	// Allocate buffer for a duration of one second
	audioData.ringBuffer.buffer = nullptr;
	audioData.ringBuffer.buffer = new float[audioData.properties.sampleRate];
	if (audioData.ringBuffer.buffer == nullptr)
		exitError("[ERROR] could not allocate audio buffer");

	PaStream* stream = nullptr;
	PaError error = Pa_OpenStream(
		&stream,
		nullptr,
		&outputParameters,
		audioData.properties.sampleRate,
		paFramesPerBufferUnspecified,
		paClipOff, // we won't output out of range samples so don't bother clipping them
		paCallback,
		&audioData // user data transfered to callback
	);

	if (error != paNoError)
		exitError("[ERROR] Port Audio : could not open stream");
	if (Pa_StartStream(stream) != paNoError)
		exitError("[ERROR] Port Audio : could not start stream");

	audioData.ringBuffer.stream = stream;
}

int main(void)
{
	//portmidi_sandbox();

	std::thread midiThread(portmidi_sandbox);
	//Pa_Sleep(2 * 1000);

	AudioData audio = {};
	audio.properties = {
		.sampleRate = 44100,
		.channels = 2,
		.sampleFormat = paFloat32
	};

	portaudio_sandbox(audio);

	while (1)
	{
	}
}
