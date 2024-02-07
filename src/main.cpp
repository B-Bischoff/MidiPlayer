#include <cmath>
#include <cstring>
#include <iostream>
#include <math.h>
#include <chrono>
#include <thread>
#include <assert.h>
#include <portmidi.h>

#include <RtAudio.h>

#define VERBOSE false

void exitError(const std::string& error)
{
	std::cerr << error << std::endl;
	exit(1);
}

struct RingBuffer {
	float* buffer; // To make true stereo, this buffer should be twice its size
	//PaStream* stream;
	unsigned int leftPhase, rightPhase;
	unsigned int writeCursor;
};

// [TODO] move everything in the RingBuffer struct
struct AudioProperties {
	unsigned int sampleRate;
	unsigned int channels;
};

struct AudioData {
	RingBuffer ringBuffer;
	AudioProperties properties;

	void incrementPhases()
	{
		ringBuffer.leftPhase = (ringBuffer.leftPhase + 2) % (properties.sampleRate * properties.channels);
		ringBuffer.rightPhase = (ringBuffer.rightPhase + 2) % (properties.sampleRate * properties.channels);
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
	double t = sinl(freqToAngularVelocity(hertz) * time);
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
	RingBuffer& rb = audio.ringBuffer;

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

		*buffer++ = rb.buffer[rb.leftPhase];
		*buffer++ = rb.buffer[rb.rightPhase];
		audio.incrementPhases();
	}
	std::cout << rb.leftPhase / 2 << " " << rb.writeCursor / 2.0f << std::endl;

	return 0;
}

int main(void)
{
	const unsigned int targetFPS = 60;
	const std::chrono::duration<double> targetFrameDuration(1.0f / (double)targetFPS);

	AudioData audio = {};
	audio.properties = {
		.sampleRate = 44100,
		.channels = 2,
		//.sampleFormat = paFloat32
	};
	RingBuffer& rb = audio.ringBuffer;
	audio.ringBuffer.rightPhase = 1; // [TODO] move this line in a constructor
	audio.ringBuffer.buffer = new float[audio.properties.sampleRate * audio.properties.channels];

	if (audio.ringBuffer.buffer == nullptr)
		exitError("[ERROR]: ring buffer allocation failed.");

	memset(audio.ringBuffer.buffer, 0, sizeof(float) * audio.properties.sampleRate * audio.properties.channels);

	RtAudio dac;
	std::vector<unsigned int> deviceIds = dac.getDeviceIds();
	if ( deviceIds.size() < 1 )
	{
		std::cout << "\nNo audio devices found!\n";
		exit( 0 );
	}
	RtAudio::StreamParameters parameters;
	parameters.deviceId = dac.getDefaultOutputDevice();
	parameters.nChannels = 2;
	parameters.firstChannel = 0;
	unsigned int sampleRate = 44100;
	unsigned int bufferFrames = sampleRate / targetFPS;
	if ( dac.openStream( &parameters, NULL, RTAUDIO_FLOAT64, sampleRate,
											 &bufferFrames, &saw, &audio ) ) {
		std::cout << '\n' << dac.getErrorText() << '\n' << std::endl;
		exit( 0 ); // problem with device settings
	}

	//assert(bufferFrames == 44100);

	if ( dac.startStream() )
	{
		std::cout << dac.getErrorText() << std::endl;
		exit(1);
	}

	//portmidi_sandbox();

	//std::thread midiThread(portmidi_sandbox);

	auto programStartTime = std::chrono::high_resolution_clock::now();

	rb.writeCursor = (44100.0f / (double)targetFPS) * 35.0f;

	rb.leftPhase = 0;
	rb.rightPhase = 1;
	while (1)
	{
		auto startTime = std::chrono::high_resolution_clock::now();

		auto programElapsedTime = std::chrono::duration_cast<std::chrono::duration<double>>(startTime - programStartTime);
		//std::cout << programElapsedTime.count() << std::endl;

		for (int i = 0; i < audio.properties.sampleRate / targetFPS; i++)
		{
			double tmp;
			double t = std::modf(programElapsedTime.count(), &tmp) + (1.0f / 44100.0f * (double)(i));
			double value = ocs(440, t) * 0.5f;

			for (int j = 0; j < audio.properties.channels; j++)
			{
				rb.buffer[rb.writeCursor] = value;
				rb.writeCursor = (rb.writeCursor + 1) % (audio.properties.sampleRate * audio.properties.channels);
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
