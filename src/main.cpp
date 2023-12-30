#include <iostream>
#include <alsa/asoundlib.h>
#include <math.h>
#include <thread>
#include <vector>
#include <chrono>

void exitError(const std::string& error)
{
	std::cerr << error << std::endl;
	exit(1);
}

struct RingBuffer {
	short buffer[44100];
	unsigned int cursor;
};

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
};

void task(snd_pcm_t* handle, RingBuffer& ringBuffer)
{
	float dt = 0.0f;
	float sum = 0.0f;
	float timeElapsed = 0.0f;
	unsigned int channels = 2;

	auto startTime = std::chrono::high_resolution_clock::now();
	while (1)
	{
		if (sum >= 1.0f / 60.0f)
		{
			sum = 0;
			//std::cout << "time sing start : " << timeElapsed << std::endl;
			//std::cout << "frames available : " << snd_strerror(snd_pcm_avail(audio.handle)) << std::endl;
			int writeReturn = snd_pcm_writei(handle, ringBuffer.buffer + ringBuffer.cursor, 44100.0f / 60.0f);
			ringBuffer.cursor = std::fmod(ringBuffer.cursor + 44100.0f / 60.0f * channels, 44100.0f);
			if (writeReturn == -EPIPE)
			{
				std::cout << "Underrun occured" << std::endl;
				// [TODO] check prepare return value
				std::cout << snd_pcm_prepare(handle) << std::endl;
			}
			else if (writeReturn < 0)
			{
				// [TODO] handle write error
				std::cout << "Alsa buffer upload error" << std::endl;
			}
			else
			{
				if (writeReturn != 44100.0f / 60.0f)
				{
					std::cout << "[WARNING] : written " << writeReturn << " instead of supposed " << (44100.0f / 60.0f) << std::endl;
				}
				//std::cout << "written: " << writeReturn << std::endl;
				//std::cout << "frames available : " << snd_pcm_avail(audio.handle) << std::endl;
				//std::cout << "--------------------" << std::endl;
			}
		}

		float newTimeElapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - startTime).count() / 1000000000.0f;

		dt = newTimeElapsed - timeElapsed;
		sum += dt;
		timeElapsed = newTimeElapsed;
	}
}

void exitError(const char* str)
{
	std::cerr << str << std::endl;
	exit(1);
}

short *sine_wave(short *buffer, size_t sample_count, unsigned int channels, int freq) {
	bool pair = true;
	for (int i = 0; i < sample_count; i++)
	{
		buffer[i] = (short)(10000 * sinf(2 * M_PI * freq * ((float)i / channels / 44100.0f)));
		buffer[i] *= 1.5f;
		if (!pair)
			buffer[i] = 0;
		//pair = !pair;
	}
	return buffer;
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

int main(void)
{
	AudioData audio;
	audio.properties = {
		.uploadPerSecond = 60,
		.sampleRate = 44100,
		.channels = 2,
		.format = SND_PCM_FORMAT_S16_LE,
		.periods = 2, // Keep that value low to quickly ear change in played audio
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
	if (snd_pcm_open(&audio.handle, "hw:CARD=NVidia,DEV=3", SND_PCM_STREAM_PLAYBACK, 0) < 0) {
	//if (snd_pcm_open(&audio.handle, "default", SND_PCM_STREAM_PLAYBACK, 0) < 0) {
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

	const float BUFFER_SIZE_MARGIN = 1.25f; // Without this increased buffer size, some output audio devices may undergo constant underrun
	snd_pcm_uframes_t desiredBufferSize = ((float)audio.properties.sampleRate / (float)audio.properties.uploadPerSecond) * BUFFER_SIZE_MARGIN;
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
	snd_pcm_sframes_t frameAvailable = snd_pcm_avail(audio.handle);
	std::cout << "first init write : " << frameAvailable << std::endl;
	snd_pcm_writei(audio.handle, ringBuffer.buffer, frameAvailable);
	frameAvailable = snd_pcm_avail(audio.handle);
	if (frameAvailable > 0)
	{
		std::cout << "second init write : " << frameAvailable << std::endl;
		snd_pcm_writei(audio.handle, ringBuffer.buffer, frameAvailable);
	}

	sine_wave(ringBuffer.buffer, audio.properties.sampleRate, audio.properties.channels, 440);
	//std::cout << snd_pcm_writei(audio.handle, buffer, 44100) << std::endl;
	//std::cout << "frames available : " << snd_pcm_avail(audio.handle) << std::endl;


	//snd_pcm_drain(audio.handle); // Block until buffer is empty
	//snd_pcm_close(audio.handle);
	// -----------------------------------------------------

	std::thread t1(task, audio.handle, std::ref(ringBuffer));

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
		 * note on/off : ev.type ==  6/7
		 * 10 : potentiometer
		 * 67 : device disconnected
		*/
		if (snd_seq_event_input_pending(sequencer, 0) <= 0)
		{
			//std::cout << (int)ev->type << std::endl;
			snd_seq_event_input(sequencer, &ev);

			if (ev->type == SND_SEQ_EVENT_NOTEON)
			{
				std::cout << (int)ev->data.note.note << " " << (int)ev->data.note.velocity << std::endl;

				// Generate sine wave samples dynamically
				sine_wave(ringBuffer.buffer, audio.properties.sampleRate, audio.properties.channels, 660);
			}
			else if (ev->type == SND_SEQ_EVENT_NOTEOFF)
			{
				sine_wave(ringBuffer.buffer, audio.properties.sampleRate, audio.properties.channels, 440);
			}
			else if (ev->type == 10) // potentiometer
			{
				std::cout << ev->data.control.channel << "value : " << ev->data.control.value << std::endl;
			}
		}
		//std::cout << timeElapsed << std::endl;
	}
}
