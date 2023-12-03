#include <iostream>
#include <alsa/asoundlib.h>
#include <math.h>
#include <pthread.h>
#include <vector>

void exitError(const char* str)
{
	std::cerr << str << std::endl;
	exit(1);
}

short *sine_wave(short *buffer, size_t sample_count, int freq) {
	for (int i = 0; i < sample_count; i++) {
		buffer[i] = 10000 * sinf(2 * M_PI * freq * ((float)i / 44100.0f));
		buffer[i] *= 0.2f;
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
	const int channels = 1;
	const snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;  // 16-bit little-endian

	// Open PCM device
	if (snd_pcm_open(&pcm_handle, "default", SND_PCM_STREAM_PLAYBACK, 0) < 0) {
		fprintf(stderr, "Error opening PCM device\n");
		return 1;
	}

	snd_pcm_hw_params_t *params;

	snd_pcm_hw_params_alloca(&params);
	snd_pcm_hw_params_any(pcm_handle, params);
	snd_pcm_hw_params_set_access(pcm_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
	snd_pcm_hw_params_set_format(pcm_handle, params, format);
	snd_pcm_hw_params_set_channels(pcm_handle, params, channels);
	snd_pcm_hw_params_set_rate(pcm_handle, params, sample_rate, 0);
	snd_pcm_hw_params_set_periods(pcm_handle, params, 10, 0);
	snd_pcm_hw_params_set_period_time(pcm_handle, params, 100000, 0); // 0.1 seconds

	// Apply parameters to PCM device
	if (snd_pcm_hw_params(pcm_handle, params) < 0) {
		fprintf(stderr, "Error setting hardware parameters\n");
		return 1;
	}

	short buffer[44100];
	memset(buffer, 0, sizeof(buffer));


	sine_wave(buffer, sample_rate, 440);

	if (snd_pcm_writei(pcm_handle, buffer, 44100) < 0)
		exitError("Error writing to PCM device");

	snd_pcm_drain(pcm_handle); // Block until buffer is empty
	snd_pcm_close(pcm_handle);
	// -----------------------------------------------------

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

			if (ev->type == 6) // note on
			{
				std::cout << (int)ev->data.note.note << " " << (int)ev->data.note.velocity << std::endl;
			}
			else if (ev->type == 10) // potentiometer
			{
				std::cout << ev->data.control.channel << "value : " << ev->data.control.value << std::endl;
			}
		}
	}
}
