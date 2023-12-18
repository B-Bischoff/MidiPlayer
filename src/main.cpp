#include <asm-generic/errno-base.h>
#include <iostream>
#include <alsa/asoundlib.h>
#include <math.h>
#include <thread>
#include <vector>
#include <chrono>

struct RingBuffer {
	short buffer[44100];
	unsigned int cursor;
};

void task(snd_pcm_t* pcm_handle, RingBuffer& ringBuffer)
{
	float dt = 0.0f;
	float sum = 0.0f;
	float timeElapsed = 0.0f;

	auto startTime = std::chrono::high_resolution_clock::now();
	while (1)
	{
		if (sum >= 1.0f / 60.0f)
		{
			sum = 0;
			//std::cout << "time sing start : " << timeElapsed << std::endl;
			//std::cout << "frames available : " << snd_strerror(snd_pcm_avail(pcm_handle)) << std::endl;
			int writeReturn = snd_pcm_writei(pcm_handle, ringBuffer.buffer + ringBuffer.cursor, 44100.0f / 60.0f);
			ringBuffer.cursor = std::fmod(ringBuffer.cursor + 44100.0f / 60.0f, 44100.0f);
			if (writeReturn == -EPIPE)
			{
				std::cout << "Underrun occured" << std::endl;
				// [TODO] check prepare return value
				snd_pcm_prepare(pcm_handle);
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
					std::cout << "[WARNING] : written " << writeReturn << std::endl;
				}
				//std::cout << "written: " << writeReturn << std::endl;
				//std::cout << "frames available : " << snd_pcm_avail(pcm_handle) << std::endl;
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

short *sine_wave(short *buffer, size_t sample_count, int freq) {
	bool pair = true;
	for (int i = 0; i < sample_count; i++)
	{
		buffer[i] = (short)(10000 * sinf(2 * M_PI * freq * ((float)i / 44100.0f)));
		buffer[i] *= 0.1f;
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
	// ----------------- PLAY SOUND TEST CODE ----------------
	snd_pcm_t *pcm_handle;
	unsigned int sample_rate = 44100;
	const int channels = 2;
	const snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;  // 16-bit little-endian
	int direction = 0;

	// Open PCM device
	if (snd_pcm_open(&pcm_handle, "default", SND_PCM_STREAM_PLAYBACK, 0) < 0) {
		fprintf(stderr, "Error opening PCM device\n");
		return 1;
	}

	//snd_pcm_nonblock(pcm_handle, 1);

	snd_pcm_hw_params_t *params;

	snd_pcm_hw_params_alloca(&params);
	snd_pcm_hw_params_any(pcm_handle, params);
	snd_pcm_hw_params_set_access(pcm_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
	snd_pcm_hw_params_set_format(pcm_handle, params, format);
	snd_pcm_hw_params_set_channels(pcm_handle, params, channels);
	snd_pcm_hw_params_set_rate(pcm_handle, params, sample_rate, direction); // use near variant

	unsigned int periods = 4;
	snd_pcm_hw_params_set_periods_near(pcm_handle, params, &periods, &direction);
	//snd_pcm_hw_params_set_period_size(pcm_handle, params, 1, 0);
	//snd_pcm_hw_params_set_period_time(pcm_handle, params, 100000, 0); // 0.1 seconds
	snd_pcm_hw_params_set_buffer_size(pcm_handle, params, ((float)sample_rate / 60.0f) * 2.0f);

	// Apply parameters to PCM device
	if (snd_pcm_hw_params(pcm_handle, params) < 0) {
		fprintf(stderr, "Error setting hardware parameters\n");
		return 1;
	}

	unsigned int val;
	int dir;

	snd_pcm_hw_params_get_channels(params, &val);
	std::cout << "channels : " << val << std::endl;
	snd_pcm_hw_params_get_rate(params, &val, &dir);
	std::cout << "rate : " << val << std::endl;
	snd_pcm_hw_params_get_periods(params, &val, &dir);
	std::cout << "periods : " << val << std::endl;
	snd_pcm_hw_params_get_period_time(params, &val, &dir);
	std::cout << "period time " << (val / 1000000.0) << " seconds" << std::endl;
	snd_pcm_uframes_t frames;
	snd_pcm_hw_params_get_period_size(params, &frames, &dir);
	std::cout << "period size " << frames << std::endl;
	snd_pcm_hw_params_get_buffer_time(params, &val, &dir);
	std::cout << "buffer time: " << (val / 1000000.0f) << " seconds" << std::endl;


	RingBuffer ringBuffer = {};

	std::cout << snd_pcm_writei(pcm_handle, ringBuffer.buffer, 44100 / 2.0f) << std::endl;
	std::cout << "frames available : " << snd_pcm_avail(pcm_handle) << std::endl;
	//std::cout << snd_pcm_writei(pcm_handle, buffer, 44100) << std::endl;
	std::cout << "frames available : " << snd_pcm_avail(pcm_handle) << std::endl;
	sine_wave(ringBuffer.buffer, sample_rate, 440);
	//std::cout << snd_pcm_writei(pcm_handle, buffer, 44100) << std::endl;
	//std::cout << "frames available : " << snd_pcm_avail(pcm_handle) << std::endl;


	//snd_pcm_drain(pcm_handle); // Block until buffer is empty
	//snd_pcm_close(pcm_handle);
	// -----------------------------------------------------

	std::thread t1(task, pcm_handle, std::ref(ringBuffer));

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
				sine_wave(ringBuffer.buffer, 44100, 660);
			}
			else if (ev->type == SND_SEQ_EVENT_NOTEOFF)
			{
				sine_wave(ringBuffer.buffer, 44100, 440);
			}
			else if (ev->type == 10) // potentiometer
			{
				std::cout << ev->data.control.channel << "value : " << ev->data.control.value << std::endl;
			}
		}
		//std::cout << timeElapsed << std::endl;
	}
}
