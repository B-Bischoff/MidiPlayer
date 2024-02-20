#include "inc.hpp"
#include "envelope.hpp"

void initInput(InputManager& inputManger) // [TODO] Should this be in a constructor ?
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
#if VERBOSE
		std::cout << i << " " << info->name << std::endl;
		std::cout << "input/output: " << info->input << " " << info->output << std::endl;
		std::cout << "open/virtual: " << info->opened << " " << info->is_virtual << std::endl;
		std::cout << std::endl;
#endif
	}

	Pm_OpenInput(&inputManger.midiStream, 3, NULL, 512, NULL, NULL);
}

void handleInput(InputManager& inputManager, std::vector<sEnvelopeADSR>& envelopes, double time)
{
	if (USE_KB_AS_MIDI_INPUT)
	{
	}
	int numEvents = Pm_Read(inputManager.midiStream, inputManager.buffer, 32);

	for (int i = 0; i < numEvents; i++)
	{
		PmEvent& event = inputManager.buffer[i];

		// Extract MIDI status and data bytes
		PmMessage message = event.message;
		int status = Pm_MessageStatus(message);
		int keyIndex = Pm_MessageData1(message);
		int velocity = Pm_MessageData2(message);

		//std::cout << " state " << status << " key " << (int)keyIndex << " vel " << (int)velocity << std::endl;
		if ((status == 145 || status == 155) && velocity != 0.0)
		{
			for (sEnvelopeADSR& e : envelopes)
			{
				if (e.phase == Phase::Inactive)
				{
					//std::cout << keyIndex << " on " << std::endl;
					e.NoteOn(time);
					e.keyIndex = keyIndex;
					break;
				}
			}
		}
		else
		{
			for (sEnvelopeADSR& e : envelopes)
			{
				if (e.keyIndex == keyIndex)
				{
					//std::cout << keyIndex << " off " << std::endl;
					e.NoteOff(time);
					break;
				}
			}
		}
	}
}
