#include <cmath>
#include <cstring>
#include <iostream>
#include <alsa/asoundlib.h>
#include <math.h>
#include <thread>
#include <vector>
#include <chrono>
#include <iomanip>

#include <portaudio.h>
#include <portmidi.h>

#define VERBOSE true

float AUDIO_DATA[200];
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

int writeBuffer(snd_pcm_t* handle, RingBuffer& ringBuffer, unsigned int size)
{
	int writeReturn = 0;
	const int channels = 2; // [TODO] pass this as parameter or with AudioData struct

	//std::cout << "cursor pos : " << ringBuffer.cursor << std::endl;
	if (ringBuffer.cursor + size >= 44100)
	{
		std::cout << "loop ---------------------------------------------------------------------------" << std::endl;
		unsigned int firstWriteSize = 44100 - ringBuffer.cursor;
		unsigned int secondWriteSize = size - firstWriteSize;

		/*
		writeReturn = snd_pcm_writei(handle, ringBuffer.buffer + ringBuffer.cursor, firstWriteSize);
		if (writeReturn == -EPIPE)
			return writeReturn;

		if (writeReturn != firstWriteSize)
		{
			writeReturn += snd_pcm_writei(handle, ringBuffer.buffer + ringBuffer.cursor + firstWriteSize - writeReturn, firstWriteSize - writeReturn);
		}

		writeReturn += snd_pcm_writei(handle, ringBuffer.buffer + ringBuffer.cursor + writeReturn, secondWriteSize);
		ringBuffer.cursor = std::fmod(ringBuffer.cursor + writeReturn * channels, 44100.0f); // [TODO] why would you use fmod ??
		ringBuffer.cursor = secondWriteSize;
		*/

		//////////////////////////////////////////

		unsigned int sum = 0;
		// Send data until buffer end
		while (sum != firstWriteSize)
		{
			writeReturn = snd_pcm_writei(handle, ringBuffer.buffer + ringBuffer.cursor, firstWriteSize - sum);
			if (writeReturn == -EPIPE)
				return writeReturn;
			ringBuffer.cursor = (ringBuffer.cursor + writeReturn * channels) % 44100;
			sum += writeReturn;
		}

		// Send remaining data
		sum = 0;
		while (sum != secondWriteSize)
		{
			writeReturn = snd_pcm_writei(handle, ringBuffer.buffer + ringBuffer.cursor, secondWriteSize - sum);
			if (writeReturn == -EPIPE)
				return writeReturn;
			ringBuffer.cursor = (ringBuffer.cursor + writeReturn * channels) % 44100;
			sum += writeReturn;
		}
		writeReturn = size;
	}
	else
	{
		std::cout << "standart" << std::endl;
		unsigned int sum = 0;
		while (sum != size)
		{
			std::cout << "x" << std::endl;
			writeReturn = snd_pcm_writei(handle, ringBuffer.buffer + ringBuffer.cursor, size - sum);
			if (writeReturn == -EPIPE)
				return writeReturn;
			ringBuffer.cursor = (ringBuffer.cursor + writeReturn * channels) % 44100;
			sum += writeReturn;
		}
	}

	return writeReturn;
}

void uploadRingBufferToAlsa(snd_pcm_t* handle, AudioData& audio)
{
	double dt = 0.0f;
	double sum = 0.0f;
	unsigned int channels = 2;
	RingBuffer& ringBuffer = audio.ringBuffer;

	ringBuffer.timeElapsed = 0.0;

	auto startTime = std::chrono::high_resolution_clock::now();

	float frameToWrite = 44100.0f / 60.0f;

	std::cout << &ringBuffer << std::endl;
	sine_wave(audio.ringBuffer, 44100.0f, 2, 440, 0);

	while (1)
	{
		if (sum >= 1.0f / 60.0f / 2.0f) // Write rate is divided by 2 to avoid underrun
		{
			if (audio.pressed)
			{
				//memset(audio.ringBuffer.buffer, 0, sizeof(audio.ringBuffer.buffer));
				sine_wave(audio.ringBuffer, 44100.0f, 2, 440, 0);
			}
			else
			{
				//std::cout << "not pressed" << std::endl;
				memset(audio.ringBuffer.buffer, 0, sizeof(audio.ringBuffer.buffer));
			}
			//std::cout << sum << std::endl;
			sum = 0;
			//std::cout << "time sing start : " << timeElapsed << std::endl;
			//std::cout << "frames available : " << snd_strerror(snd_pcm_avail(audio.handle)) << std::endl;
			int writeReturn;
			int frameAvailable = snd_pcm_avail(handle);
			//std::cout << "write return                            " << writeReturn << std::endl;

			//for (int i = 0; i < 44100.0f / 60.0f; i++)
			//	std::cout << ringBuffer.buffer[ringBuffer.cursor + i] << " ";
			//std::cout << std::endl;

			if (frameAvailable > frameToWrite)
			{
				std::cout << "[preventing underrun]" << std::endl;
				frameToWrite = frameAvailable;
				writeReturn = writeBuffer(handle, ringBuffer, frameToWrite);
				frameToWrite = 44100.0f / 60.0f;
			}
			else
			{
				writeReturn = writeBuffer(handle, ringBuffer, frameToWrite);
			}

			if (writeReturn == -EPIPE)
			{
				//std::cout << "Underrun occured" << std::endl;
				std::cout << "prepare : " << snd_pcm_prepare(handle) << std::endl;

				short silenceBuffer[44100] = {};
				snd_pcm_sframes_t frameAvailable = snd_pcm_avail(handle);
				std::cout << "first init write : " << frameAvailable << std::endl;
				snd_pcm_writei(handle, silenceBuffer, frameAvailable);
				frameAvailable = snd_pcm_avail(handle);
				while (frameAvailable > 0)
				{
					std::cout << "second init write : " << frameAvailable << std::endl;
					snd_pcm_writei(handle, silenceBuffer, frameAvailable);
					frameAvailable = snd_pcm_avail(handle);
				}
			}
			else if (writeReturn < 0)
			{
				// [TODO] handle write error
				std::cout << "Alsa buffer upload error" << std::endl;
				exit(1);
			}
			else
			{
				if (writeReturn < 44100.0f / 60.0f)
				{
					std::cout << "[WARNING] : written " << writeReturn << " instead of supposed " << (44100.0f / 60.0f) << std::endl;
				}
			}
			std::cout << (int)ringBuffer.cursor << " " << (int)ringBuffer.writeCursor << std::endl;
		}

		std::this_thread::sleep_for(std::chrono::nanoseconds(1));
		double newTimeElapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - startTime).count() / 1000000000.0f;
		dt = newTimeElapsed - ringBuffer.timeElapsed;
		sum += dt;
		ringBuffer.timeElapsed = newTimeElapsed;
	}
}

void generateSound(AudioData& audio)
{
	double dt = 0.0f;
	double sum = 0.0f;
	auto startTime = std::chrono::high_resolution_clock::now();
	double previousFrame = 0;

	std::cout << &audio.ringBuffer << std::endl;

	while (1)
	{
		if (sum >= 1.0f / 60.0f / 2.0f)
		{
			sum = 0;

			/*
			if (audio.pressed)
			{
				sine_wave(audio.ringBuffer, 44100.0f / 60.0f, 2, 440, audio.ringBuffer.timeElapsed);
				//sine_wave(audio.ringBuffer.buffer, 44100.0f, 2, 440, 0);
			}
			else
			{
				//std::cout << "not pressed" << std::endl;
				memset(audio.ringBuffer.buffer, 0, sizeof(audio.ringBuffer.buffer));
			}
			*/
		}
		double newTimeElapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - startTime).count() / 1000000000.0f;
		dt = newTimeElapsed - previousFrame;
		sum += dt;
		previousFrame = newTimeElapsed;
		std::this_thread::sleep_for(std::chrono::nanoseconds(1));
	}
}

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

std::string getDevicePort(snd_seq_t* sequencer)
{
	// List clients
	puts("Id Port    Client name                      Port name");

	snd_seq_port_info_t *pinfo;
	snd_seq_client_info_t *cinfo;

	snd_seq_port_info_alloca(&pinfo);
	snd_seq_client_info_alloca(&cinfo);

	snd_seq_client_info_set_client(cinfo, -1);
	unsigned int clientIndex = 1;
	std::vector<std::string> clientsPort;

	while (snd_seq_query_next_client(sequencer, cinfo) >= 0)
	{
		int client = snd_seq_client_info_get_client(cinfo);

		snd_seq_port_info_set_client(pinfo, client);
		snd_seq_port_info_set_port(pinfo, -1);
		while (snd_seq_query_next_port(sequencer, pinfo) >= 0)
		{
			if ((snd_seq_port_info_get_capability(pinfo) & (SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ)) != (SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ))
					continue;
			printf("%2d %3d:%-3d  %-32.32s %s\n", clientIndex, snd_seq_port_info_get_client(pinfo), snd_seq_port_info_get_port(pinfo), snd_seq_client_info_get_name(cinfo), snd_seq_port_info_get_name(pinfo));

			std::string portStr = std::to_string(snd_seq_port_info_get_client(pinfo)) + ":" + std::to_string(snd_seq_port_info_get_port(pinfo));
			clientsPort.push_back(portStr);
			clientIndex++;
		}
	}

	std::string selectedPort = "";

	while (selectedPort.empty())
	{
		unsigned int selectedId = 0;

		std::cout << std::endl << "Select Id to use " << std::endl << ">> ";
		std::cin >> selectedId;

		try
		{
			selectedPort = clientsPort.at(selectedId - 1);
		}
		catch (std::out_of_range& e)
		{
			std::cout << "invalid Id" << std::endl;
		}
	}

	std::cout << std::endl << "---------------------------" << std::endl << std::endl;

	return selectedPort;
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

	static double timeElapsed = 0.0f;
	timeElapsed += timeInfo->outputBufferDacTime;
	if ((int)timeElapsed % 2)
	{
		for( int i=0; i < 200; i++ )
		{
			AUDIO_DATA[i] = 0.1f * (float) sin( ((double)i/(double)200) * M_PI * 2. );
		}
	}
	else
	{
		for( int i=0; i < 200; i++ )
		{
			AUDIO_DATA[i] = 0.1f * (float) sin( ((double)i/(double)400) * M_PI * 2. );
		}
	}

	std::cout << timeElapsed << std::endl;

	for (int i = 0; i < 64; i++)
	{
		*out++ = AUDIO_DATA[leftPhase];
		*out++ = AUDIO_DATA[rightPhase];
		leftPhase = (leftPhase + 1) % 200;
		rightPhase = (rightPhase + 1) % 200;
	}

	return paContinue;
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

	std::cout << "Device id : " << defaultDevice << std::endl;
	printPaDeviceInfo(Pa_GetDeviceInfo(defaultDevice));

	PaStreamParameters outputParameters;
	outputParameters.device = defaultDevice;
	outputParameters.channelCount = 2;
	outputParameters.sampleFormat = paFloat32;
	outputParameters.suggestedLatency = Pa_GetDeviceInfo(defaultDevice)->defaultLowOutputLatency;
	outputParameters.hostApiSpecificStreamInfo = nullptr;

	void* data = nullptr;

	memset(AUDIO_DATA, 0, sizeof(AUDIO_DATA));
	for( int i=0; i < 200; i++ )
	{
		AUDIO_DATA[i] = 0.1f * (float) sin( ((double)i/(double)200) * M_PI * 2. );
	}
	leftPhase = 0;
	rightPhase = 0;

	PaStream* stream = nullptr;
	PaError error = Pa_OpenStream(
		&stream,
		nullptr, // input parameters (not used)
		&outputParameters,
		44100,
		64,
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
    // Initialize PortMidi
    Pm_Initialize();

    // Get the number of MIDI devices
    int numDevices = Pm_CountDevices();
    if (numDevices <= 0) {
        std::cerr << "No MIDI devices found." << std::endl;
        return 1;
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

    return 0;
	portaudio_sandbox();
	while (1) {}
	//Pa_Sleep(5  * 1000 );
	return 0;

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

	void** hints = nullptr;
	if (snd_device_name_hint(-1, "pcm", &hints) < 0)
	{
		std::cerr << "Error getting hints" << std::endl;
		return 1;
	}

	for (int i = 0; hints[i] != nullptr; i++)
	{
		char* description = snd_device_name_get_hint(hints[i], "DESC");
		char* name = snd_device_name_get_hint(hints[i], "NAME");
		if (name)
		{
			std::cout << "Device " << i << " " << name << " : " << description << std::endl;
			free(name);
		}
	}
	snd_device_name_free_hint(hints);

	// Open PCM device
	//if (snd_pcm_open(&audio.handle, "hw:CARD=NVidia,DEV=3", SND_PCM_STREAM_PLAYBACK, 0) < 0)
	if (snd_pcm_open(&audio.handle, "default", SND_PCM_STREAM_PLAYBACK, 0) < 0)
	{
		std::cerr << "Error opening PCM device" << std::endl;
		return 1;
	}

	//snd_pcm_nonblock(audio.handle, 1);

	snd_pcm_hw_params_t *params;
	unsigned int val;
	int dir;

	snd_pcm_hw_params_alloca(&params);
	if (snd_pcm_hw_params_any(audio.handle, params) < 0)
		exitError("snd_pcm_hw_params_any");
	if (snd_pcm_hw_params_set_access(audio.handle, params, SND_PCM_ACCESS_RW_INTERLEAVED) < 0)
		exitError("snd_pcm_hw_params_set_access");
	if (snd_pcm_hw_params_set_format(audio.handle, params, audio.properties.format) < 0)
		exitError("snd_pcm_hw_params_set_format");
	if (snd_pcm_hw_params_set_channels(audio.handle, params, audio.properties.channels) < 0)
		exitError("snd_pcm_hw_params_set_channels");
	if (snd_pcm_hw_params_set_rate_near(audio.handle, params, &audio.properties.sampleRate, &audio.properties.direction) < 0) // use near variant
		exitError("snd_pcm_hw_params_set_rate");
	if (snd_pcm_hw_params_get_rate(params, &val, &dir) < 0)
		exitError("snd_pcm_hw_params_get_rate");
	std::cout << "rate : " << val << std::endl;
	assert(val == audio.properties.sampleRate && "sample rate not available");

	if (snd_pcm_hw_params_get_channels(params, &val) < 0)
		exitError("snd_pcm_hw_params_get_channels");
	std::cout << "channels : " << val << std::endl;
	assert (val == audio.properties.channels && "requested channel number not available");

	snd_pcm_hw_params_set_periods_near(audio.handle, params, &audio.properties.periods, &audio.properties.direction);
	if (snd_pcm_hw_params_get_periods(params, &val, &dir) < 0)
		exitError("snd_pcm_hw_params_get_periods");
	std::cout << "periods : " << val << std::endl;
	unsigned int definitivePeriod = val;
	unsigned int periodTimeInMicroSeconds = ((1.0f / 60.0f) / (float)definitivePeriod) * 1000000;
	std::cout << "----------> " << periodTimeInMicroSeconds << std::endl;
	if (snd_pcm_hw_params_set_period_time_min(audio.handle, params, &periodTimeInMicroSeconds, &audio.properties.direction) < 0)
		exitError("snd_pcm_hw_params_set_period_time_near");

	snd_pcm_uframes_t desiredBufferSize = ((float)audio.properties.sampleRate / (float)audio.properties.uploadPerSecond);
	std::cout << "desired buffer size " << desiredBufferSize << std::endl;
	if (snd_pcm_hw_params_set_buffer_size_near(audio.handle, params, &desiredBufferSize) < 0)
		exitError("snd_pcm_hw_params_set_buffer_size_near");

	// Apply parameters to PCM device
	if (snd_pcm_hw_params(audio.handle, params) < 0)
		exitError("Error setting hardware parameters");

	snd_pcm_uframes_t frames;
	if (snd_pcm_hw_params_get_period_size(params, &frames, &dir) < 0)
		exitError("snd_pcm_hw_params_get_period_size");
	std::cout << "period size " << frames << std::endl;

	unsigned a1;
	int a2;
	if (snd_pcm_hw_params_get_buffer_time(params, &val, &dir) < 0)
		exitError("snd_pcm_hw_params_get_buffer_time");
	std::cout << "buffer time: " << (val / 1000000.0f) << " seconds" << std::endl;

	if (snd_pcm_hw_params_get_periods(params, &val, &dir) < 0)
		exitError("snd_pcm_hw_params_get_periods");
	std::cout << "periods : " << val << std::endl;

	RingBuffer ringBuffer = {};

	// Fill alsa buffer with silence
	short silenceBuffer[44100] = {};
	snd_pcm_sframes_t frameAvailable = snd_pcm_avail(audio.handle);
	std::cout << "first init write : " << frameAvailable << std::endl;
	snd_pcm_writei(audio.handle, silenceBuffer, frameAvailable);
	frameAvailable = snd_pcm_avail(audio.handle);
	while (frameAvailable > 0)
	{
		std::cout << "second init write : " << frameAvailable << std::endl;
		snd_pcm_writei(audio.handle, silenceBuffer, frameAvailable);
		frameAvailable = snd_pcm_avail(audio.handle);
	}

	//sine_wave(ringBuffer.buffer, audio.properties.sampleRate, audio.properties.channels, 440);
	//std::cout << snd_pcm_writei(audio.handle, buffer, 44100) << std::endl;
	//std::cout << "frames available : " << snd_pcm_avail(audio.handle) << std::endl;


	//snd_pcm_drain(audio.handle); // Block until buffer is empty
	//snd_pcm_close(audio.handle);
	// -----------------------------------------------------

	std::thread t1(uploadRingBufferToAlsa, audio.handle, std::ref(audio));
	std::thread generateSoundThread(generateSound, std::ref(audio));

	snd_seq_t* sequencer;

	if (snd_seq_open(&sequencer, "default", SND_SEQ_OPEN_INPUT, 0) < 0)
		exitError("snd_seq_open");

	std::string devicePort = getDevicePort(sequencer);

	snd_seq_set_client_name(sequencer, "MidiKeyboard");

	int port = snd_seq_create_simple_port(sequencer, "MyMIDIPort", SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE, SND_SEQ_PORT_TYPE_APPLICATION);

	snd_seq_addr_t addr;
	if (snd_seq_parse_address(sequencer, &addr, devicePort.c_str()))
		exitError("seq parse address");

	if (snd_seq_connect_from(sequencer, port, addr.client, addr.port))
		exitError("connect to");

	snd_seq_event_t* ev;
	snd_seq_event_input(sequencer, &ev);

	while (1)
	{
		/*
		* note on/off : ev.type == 6/7
		* 10 : potentiometer
		* 67 : device disconnected
		*/
		if (snd_seq_event_input_pending(sequencer, 0) >= 0)
		{
			//std::cout << (int)ev->type << std::endl;
			snd_seq_event_input(sequencer, &ev);

			if (ev->type == SND_SEQ_EVENT_NOTEON)
			{
				std::cout << "[NOTE ON] " << (int)ev->data.note.note << " " << (int)ev->data.note.velocity << std::endl;

				float hertzValue =  440.0 * std::pow(2.0, (ev->data.note.note - 49) / 12.0);
				std::cout << "Hz : " << hertzValue << std::endl;
				//sine_wave(ringBuffer.buffer, audio.properties.sampleRate, audio.properties.channels, hertzValue);
				audio.pressed = true;
			}
			else if (ev->type == SND_SEQ_EVENT_NOTEOFF)
			{
				std::cout << "[NOTE OFF] " << (int)ev->data.note.note << std::endl;
				audio.pressed = false;
				//memset(ringBuffer.buffer, 0, sizeof(ringBuffer.buffer));
				//sine_wave(ringBuffer.buffer, audio.properties.sampleRate, audio.properties.channels, 440);
			}
			else if (ev->type == 10) // potentiometer
			{
				std::cout << ev->data.control.channel << "value : " << ev->data.control.value << std::endl;
			}
		}
		std::cout << ringBuffer.timeElapsed << std::endl;
	}
}
