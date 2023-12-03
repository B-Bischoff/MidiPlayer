#include <iostream>
#include <alsa/asoundlib.h>
#include <math.h>
#include <vector>

void exitError(const char* str)
{
	std::cerr << str << std::endl;
	exit(1);
}

// Function to generate a sine wave for a given frequency and duration
void generate_sine_wave(double frequency, double duration, char *buffer, int sample_rate)
{
	int num_frames = duration * sample_rate;
	double angular_frequency = 2.0 * M_PI * frequency / sample_rate;

	for (int i = 0; i < num_frames; ++i)
	{
		double t = (double)i / sample_rate;
		double sample = sin(angular_frequency * t);
		int16_t pcm_value = (int16_t)(32767.0 * sample);  // 16-bit PCM value
		buffer[i * 2] = pcm_value & 0xFF;
		buffer[i * 2 + 1] = (pcm_value >> 8) & 0xFF;
	}
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
	// Define variables
	snd_pcm_t *pcm_handle;
	snd_pcm_hw_params_t *params;
	unsigned int sample_rate = 44100;
	int channels = 2;
	snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;  // 16-bit little-endian
	snd_pcm_uframes_t buffer_size = 1024;

	/*
	// ----------------- PLAY SOUND TEST CODE ----------------
	// Open PCM device
	if (snd_pcm_open(&pcm_handle, "default", SND_PCM_STREAM_PLAYBACK, 0) < 0) {
		fprintf(stderr, "Error opening PCM device\n");
		return 1;
	}

	// Allocate and initialize parameters object
	snd_pcm_hw_params_alloca(&params);
	snd_pcm_hw_params_any(pcm_handle, params);
	snd_pcm_hw_params_set_access(pcm_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
	snd_pcm_hw_params_set_format(pcm_handle, params, format);
	snd_pcm_hw_params_set_channels(pcm_handle, params, channels);
	snd_pcm_hw_params_set_rate_near(pcm_handle, params, &sample_rate, 0);
	snd_pcm_hw_params_set_buffer_size_near(pcm_handle, params, &buffer_size);

	// Apply parameters to PCM device
	if (snd_pcm_hw_params(pcm_handle, params) < 0) {
		fprintf(stderr, "Error setting hardware parameters\n");
		return 1;
	}

	// Write PCM data to the device (replace with your own audio data)
	// For simplicity, this example writes silence
	char buffer[sample_rate];  // 2 channels, 16-bit per sample
	memset(buffer, 0, sizeof(buffer));


	// Generate and play the C major scale
	double frequencies[] = {261.63, 293.66, 329.63, 349.23, 392.00, 440.00, 493.88, 523.25};  // C4 to C5
	double duration = 0.5;  // seconds

	while (1)
	for (int i = 0; i < 8; ++i) {
		generate_sine_wave(frequencies[i], duration, buffer, sample_rate);

		// Write PCM data to the device
		if (snd_pcm_writei(pcm_handle, buffer, buffer_size) < 0) {
			fprintf(stderr, "Error writing to PCM device\n");
			break;
		}
	}
	// Close PCM device
	snd_pcm_close(pcm_handle);
	*/

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
		if (snd_seq_event_input_pending(sequencer, 0) <= 0)
		{
			std::cout << "reading event" << std::endl;

			snd_seq_event_input(sequencer, &ev);
		}
	}
}
