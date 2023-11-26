#include <iostream>
#include <alsa/asoundlib.h>

void exitError(const char* str)
{
	std::cerr << str << std::endl;
	exit(1);
}

int main(void)
{
	snd_seq_t* sequencer;

	if (snd_seq_open(&sequencer, "default", SND_SEQ_OPEN_INPUT, 0) < 0)
		exitError("snd_seq_open");

	// List clients
	snd_seq_client_info_t *cinfo;
	snd_seq_port_info_t *pinfo;

	snd_seq_client_info_alloca(&cinfo);
	snd_seq_port_info_alloca(&pinfo);

	puts(" Port    Client name                      Port name");

	snd_seq_client_info_set_client(cinfo, -1);
	while (snd_seq_query_next_client(sequencer, cinfo) >= 0) {
			int client = snd_seq_client_info_get_client(cinfo);

			snd_seq_port_info_set_client(pinfo, client);
			snd_seq_port_info_set_port(pinfo, -1);
			while (snd_seq_query_next_port(sequencer, pinfo) >= 0) {
					if ((snd_seq_port_info_get_capability(pinfo)
						 & (SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ))
						!= (SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ))
							continue;
					printf("%3d:%-3d  %-32.32s %s\n",
						   snd_seq_port_info_get_client(pinfo),
						   snd_seq_port_info_get_port(pinfo),
						   snd_seq_client_info_get_name(cinfo),
						   snd_seq_port_info_get_name(pinfo));
			}
	}
	// ------------------------

	snd_seq_set_client_name(sequencer, "MidiKeyboardTest");

	int port = snd_seq_create_simple_port(sequencer, "MyMIDIPort", SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE, SND_SEQ_PORT_TYPE_APPLICATION);

	snd_seq_addr_t addr;
	if (snd_seq_parse_address(sequencer, &addr, "32:0"))
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

/*
int main()
{
	snd_seq_t* seq;
	if (snd_seq_open(&seq, "default", SND_SEQ_OPEN_INPUT, 0) < 0)
	{
		std::cerr << "Error: Cannot open sequencer." << std::endl;
		return 1;
	}

	// List clients
	snd_seq_client_info_t *cinfo;
	snd_seq_port_info_t *pinfo;

	snd_seq_client_info_alloca(&cinfo);
	snd_seq_port_info_alloca(&pinfo);

	puts(" Port    Client name                      Port name");

	snd_seq_client_info_set_client(cinfo, -1);
	while (snd_seq_query_next_client(seq, cinfo) >= 0) {
			int client = snd_seq_client_info_get_client(cinfo);

			snd_seq_port_info_set_client(pinfo, client);
			snd_seq_port_info_set_port(pinfo, -1);
			while (snd_seq_query_next_port(seq, pinfo) >= 0) {
					if ((snd_seq_port_info_get_capability(pinfo)
						 & (SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ))
						!= (SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ))
							continue;
					printf("%3d:%-3d  %-32.32s %s\n",
						   snd_seq_port_info_get_client(pinfo),
						   snd_seq_port_info_get_port(pinfo),
						   snd_seq_client_info_get_name(cinfo),
						   snd_seq_port_info_get_name(pinfo));
			}
	}
	// ------------------------

	int client = snd_seq_client_id(seq);
	int port = snd_seq_create_simple_port(seq, "MIDI-In", SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE, SND_SEQ_PORT_TYPE_APPLICATION);
	if (port < 0)
	{
		std::cerr << "Error: Cannot create MIDI port." << std::endl;
		return 1;
	}
	std::cout << "port " << port << std::endl;
	std::cout << "client " << client << std::endl;

	if (snd_seq_connect_to(seq, port, client, 24) < 0)
	{
		std::cerr << "Error: Cannot connect to sequencer." << std::endl;
		return 1;
	}

	while (true)
	{
		snd_seq_event_t *ev;
		snd_seq_event_input(seq, &ev);

		if (ev->type == SND_SEQ_EVENT_NOTEON)
		{
			std::cout << "Note On: " << ev->data.note.note << std::endl;
		}
		else if (ev->type == SND_SEQ_EVENT_NOTEOFF)
		{
			std::cout << "Note Off: " << ev->data.note.note << std::endl;
		}
		else
		{
			std::cout << "Else " << std::endl;
		}
	}

	return 0;
}
*/
