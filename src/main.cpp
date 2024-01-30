#include <cmath>
#include <cstring>
#include <iostream>
#include <alsa/asoundlib.h>
#include <math.h>
#include <thread>

#include <portaudio.h>
#include <portmidi.h>

#define VERBOSE true

#define AUDIO_SIZE 44100

float AUDIO_DATA[AUDIO_SIZE];
int leftPhase, rightPhase;

void exitError(const std::string& error)
{
	std::cerr << error << std::endl;
	exit(1);
}

struct RingBuffer {
	short buffer[44100];
	unsigned int cursor;
	unsigned int writeCursor;
	double timeElapsed;
	// [TODO] add buffer size
};

void sine_wave(RingBuffer& ringBuffer, size_t sample_count, unsigned int channels, int freq, double time);

struct AudioProperties {
	unsigned int uploadPerSecond;
	unsigned int sampleRate;
	unsigned int channels;
	snd_pcm_format_t format;
	unsigned int periods;
	int direction;
};

struct AudioData {
	RingBuffer ringBuffer;
	AudioProperties properties;
	snd_pcm_t* handle;
	bool pressed;
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

void sine_wave(RingBuffer& ringBuffer, size_t sample_count, unsigned int channels, int freq, double time)
{
	bool pair = true;
	for (int i = 0; i < sample_count; i++)
	{
		int bufferIndex = (ringBuffer.writeCursor + i) % 44100;
		//buffer[i] = (short)(10000 * sinf(2 * M_PI * freq * ((float)i / channels / 44100.0f)));
		ringBuffer.buffer[bufferIndex] = (short)ocs(freq, time + ((float)i / channels) / 44100.0f);
		ringBuffer.buffer[bufferIndex] *= 0.5f;
		if (!pair)
			ringBuffer.buffer[bufferIndex] = 0;
		//pair = !pair;
	}
	int newWriteCursorPos = (ringBuffer.writeCursor + sample_count) % 44100;
	//if (ringBuffer.writeCursor < ringBuffer.cursor && newWriteCursorPos >= ringBuffer.cursor)
	//	std::cout << "[WARNING] write cursor" << std::endl;
	ringBuffer.writeCursor = newWriteCursorPos;
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
	float* out = (float*)outputBuffer;

	for(int i = 0; i < AUDIO_SIZE; i++)
	{
		AUDIO_DATA[i] = 0.00005f * ocs(440, (float)i / 44100.0f);
	}

	std::cout << "frames per buffer : " << framesPerBuffer <<  std::endl;
	std::cout << "time elapsed: " << timeInfo->inputBufferAdcTime << std::endl;

	for (int i = 0; i < framesPerBuffer; i++)
	{
		*out++ = AUDIO_DATA[leftPhase];
		*out++ = AUDIO_DATA[rightPhase];
		leftPhase = (leftPhase + 1) % AUDIO_SIZE;
		rightPhase = (rightPhase + 1) % AUDIO_SIZE;
		if (leftPhase == 0)
			std::cout << "LEFT PHASE LOOP" << std::endl;
		if (rightPhase == 0)
			std::cout << "RIGHT PHASE LOOP" << std::endl;
	}

	return paContinue;
}

void portmidi_sandbox()
{
	// Initialize PortMidi
	Pm_Initialize();

	// Get the number of MIDI devices
	int numDevices = Pm_CountDevices();
	if (numDevices <= 0) {
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

	// Open the first available MIDI input device
	PmStream* midiStream;
	Pm_OpenInput(&midiStream, 3, NULL, 512, NULL, NULL);

	PmEvent buffer[32];

	// Your program will run here, waiting for MIDI events
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

	// Close the MIDI stream when done
	Pm_Close(midiStream);

	// Terminate PortMidi
	Pm_Terminate();
}

void portaudio_sandbox()
{
	if (Pa_Initialize() != paNoError)
	{
		std::cerr << "[ERROR] Port Audio : initialization failed" << std::endl;
		exit(1);
	}

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
	{
		std::cerr << "[ERROR] Port Audio : no default output device found" << std::endl;
		exit(1);
	}

	std::cout << "(default) Device id : " << defaultDevice << std::endl;
	printPaDeviceInfo(Pa_GetDeviceInfo(defaultDevice));

	PaStreamParameters outputParameters;
	outputParameters.device = defaultDevice;
	outputParameters.channelCount = 2;
	outputParameters.sampleFormat = paFloat32;
	outputParameters.suggestedLatency = Pa_GetDeviceInfo(defaultDevice)->defaultLowOutputLatency;
	outputParameters.hostApiSpecificStreamInfo = nullptr;

	void* data = nullptr;

	memset(AUDIO_DATA, 0, sizeof(AUDIO_DATA));
	for(int i = 0; i < AUDIO_SIZE; i++)
	{
		AUDIO_DATA[i] = 0;
		//AUDIO_DATA[i] = 0.1f * (float) sin( ((double)i/(double)200) * M_PI * 2. );
	}

	leftPhase = 0;
	rightPhase = 0;

	PaStream* stream = nullptr;
	PaError error = Pa_OpenStream(
		&stream,
		nullptr, // input parameters (not used)
		&outputParameters,
		44100,
		paFramesPerBufferUnspecified,
		paClipOff, // we won't output out of range samples so don't bother clipping them
		paCallback,
		data
	);

	if (error != paNoError)
	{
		std::cerr << "[ERROR] Port Audio : could not open stream" << std::endl;
		exit(1);
	}
	if (Pa_StartStream(stream) != paNoError)
	{
		std::cerr << "[ERROR] Port Audio : could not start stream" << std::endl;
		exit(1);
	}
}

int main(void)
{
	//portmidi_sandbox();

	std::thread midiThread(portmidi_sandbox);
	portaudio_sandbox();
	//Pa_Sleep(2 * 1000);

	AudioData audio;
	audio.properties = {
		.uploadPerSecond = 60,
		.sampleRate = 44100,
		.channels = 2,
		.format = SND_PCM_FORMAT_S16_LE,
		.periods = 3, // Keep that value low to quickly ear change in played audio
		.direction = 0,
	};
	memset(&audio.ringBuffer, 0, sizeof(RingBuffer));

	while (1)
	{
	}
}
