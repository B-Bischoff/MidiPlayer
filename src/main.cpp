#include <cmath>
#include <cstring>
#include <iostream>
#include <math.h>
#include <chrono>
#include <thread>
#include <assert.h>
#include <portaudio.h>
#include <portmidi.h>

#include <rtaudio/RtAudio.h>

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
};

struct AudioProperties {
	unsigned int sampleRate;
	unsigned int channels;
	//PaSampleFormat sampleFormat; // typedef to unsigned long
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

double freqToAngularVelocity(float hertz)
{
	return hertz * 2.0 * M_PI;
}

double ocs(float hertz, float time)
{
	double t = sinl(freqToAngularVelocity(hertz) * time);
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

//static int paCallback(const void* inputBuffer, void* outputBuffer,
//		unsigned long framesPerBuffer,
//		const PaStreamCallbackTimeInfo* timeInfo,
//		PaStreamCallbackFlags statusFlags,
//		void* data)
//{
//	AudioData& audioData = *(AudioData*)data;
//	RingBuffer& ringBuffer = audioData.ringBuffer;
//	float* out = (float*)outputBuffer;
//
//	for(int i = 0; i < audioData.properties.sampleRate; i++)
//	{
//		ringBuffer.buffer[i] = 0.00005f * ocs(440, (float)i / 44100.0f);
//	}
//
//	//std::cout << "frames per buffer : " << framesPerBuffer <<	std::endl;
//	//std::cout << "time elapsed: " << timeInfo->inputBufferAdcTime << std::endl;
//
//	for (int i = 0; i < framesPerBuffer; i++)
//	{
//		*out++ = ringBuffer.buffer[ringBuffer.leftPhase];
//		*out++ = ringBuffer.buffer[ringBuffer.rightPhase];
//		ringBuffer.leftPhase = (ringBuffer.leftPhase + 1) % audioData.properties.sampleRate;
//		ringBuffer.rightPhase = (ringBuffer.rightPhase + 1) % audioData.properties.sampleRate;
//	}
//
//	return paContinue;
//}

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

//void portaudio_sandbox(AudioData& audioData)
//{
//	if (Pa_Initialize() != paNoError)
//		exitError("[ERROR] Port Audio : initialization failed");
//
//	int numDevices = Pa_GetDeviceCount();
//
//	for (int deviceIndex = 0; deviceIndex < numDevices; deviceIndex++)
//	{
//		const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(deviceIndex);
//#if VERBOSE
//		std::cout << "Device id : " << deviceIndex << std::endl;
//		printPaDeviceInfo(deviceInfo);
//#endif
//	}
//
//	int defaultDevice = Pa_GetDefaultOutputDevice();
//	if (defaultDevice == paNoDevice)
//		exitError("[ERROR] Port Audio : no default output device found" );
//
//	std::cout << "(default) Device id : " << defaultDevice << std::endl;
//	printPaDeviceInfo(Pa_GetDeviceInfo(defaultDevice));
//
//	PaStreamParameters outputParameters;
//	outputParameters.device = defaultDevice;
//	outputParameters.channelCount = audioData.properties.channels;
//	outputParameters.sampleFormat = audioData.properties.sampleFormat;
//	outputParameters.suggestedLatency = Pa_GetDeviceInfo(defaultDevice)->defaultLowOutputLatency; // [TODO] This parameter makes the program CPU hogging, check if this can be improved
//	outputParameters.hostApiSpecificStreamInfo = nullptr;
//
//	// Allocate buffer for a duration of one second
//	audioData.ringBuffer.buffer = nullptr;
//	audioData.ringBuffer.buffer = new float[audioData.properties.sampleRate];
//	if (audioData.ringBuffer.buffer == nullptr)
//		exitError("[ERROR] could not allocate audio buffer");
//
//	PaStream* stream = nullptr;
//	PaError error = Pa_OpenStream(
//		&stream,
//		nullptr,
//		&outputParameters,
//		audioData.properties.sampleRate,
//		paFramesPerBufferUnspecified,
//		paClipOff, // we won't output out of range samples so don't bother clipping them
//		paCallback,
//		&audioData // user data transfered to callback
//	);
//
//	if (error != paNoError)
//		exitError("[ERROR] Port Audio : could not open stream");
//	if (Pa_StartStream(stream) != paNoError)
//		exitError("[ERROR] Port Audio : could not start stream");
//
//	audioData.ringBuffer.stream = stream;
//}

// Two-channel sawtooth wave generator.
int saw( void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
				 double streamTime, RtAudioStreamStatus status, void *userData )
{
	double *buffer = (double *) outputBuffer;

	//std::cout << "callback time : " << streamTime << std::endl;
	if (status)
		std::cout << "Stream underflow detected!" << std::endl;

	for (int i = 0 ; i<nBufferFrames; i++)
	{
		double tmp;
		// When using the whole stream time (decimal part included), audio produces
		// a weird sound amplified when stream time reaches a power of 2.
		// This error is probably (not 100% sure) due to floating point precision.
		double t = std::modf(streamTime, &tmp) + (1.0f / 44100.0f * (double)(i));
		double value = ocs(440, t) * 0.5f;

		*buffer++ = value; // left ear
		*buffer++ = value; // right ear
	}

	return 0;
}

int main(void)
{
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
	unsigned int bufferFrames = 44100; // buffer size
	if ( dac.openStream( &parameters, NULL, RTAUDIO_FLOAT64, sampleRate,
											 &bufferFrames, &saw, NULL ) ) {
		std::cout << '\n' << dac.getErrorText() << '\n' << std::endl;
		exit( 0 ); // problem with device settings
	}

	assert(bufferFrames == 44100);
	// Stream is open ... now start it.
	if ( dac.startStream() ) {
		std::cout << dac.getErrorText() << std::endl;
		exit(1);
	}
	/*
	char input;
	std::cout << "\nPlaying ... press <enter> to quit.\n";
	std::cin.get( input );
	// Block released ... stop the stream
	if ( dac.isStreamRunning() )
		dac.stopStream();	// or could call dac.abortStream();
 cleanup:
	if ( dac.isStreamOpen() ) dac.closeStream();
	return 0;
	*/

	//portmidi_sandbox();

	//std::thread midiThread(portmidi_sandbox);
	//Pa_Sleep(2 * 1000);

	AudioData audio = {};
	audio.properties = {
		.sampleRate = 44100,
		.channels = 2,
		//.sampleFormat = paFloat32
	};

	//portaudio_sandbox(audio);

	const unsigned int targetFPS = 144;
	const std::chrono::duration<double> targetFrameDuration(1.0f / (double)targetFPS);

	auto programStartTime = std::chrono::high_resolution_clock::now();

	while (1)
	{
		auto startTime = std::chrono::high_resolution_clock::now();

		auto programElapsedTime = std::chrono::duration_cast<std::chrono::duration<double>>(startTime - programStartTime);
		std::cout << programElapsedTime.count() << std::endl;


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
