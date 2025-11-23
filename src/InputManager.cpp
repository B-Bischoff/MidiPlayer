#include "InputManager.hpp"

InputManager::InputManager()
	: _midiEvents(255), _midiDeviceUsed(""), _midiStream(nullptr), _midiDeviceCount(0)
{
	pollMidiDevices(false);

	// Setup a callback to get mods (ctrl, shift, ...) key state
	// Others key state are obtained using glfwGetKey()
}

InputManager::~InputManager()
{
	Pm_Close(_midiStream);
	Pm_Terminate();
}

void InputManager::updateKeysState(const MidiPlayerSettings& settings, std::vector<MidiInfo>& keyPressed)
{
	if (_midiStream != nullptr)
	{
		_midiEvents.readNewEvents(_midiStream);
		const std::vector<PmEvent>& events = _midiEvents.getEvents();

		for (const PmEvent& event : events)
		{
			// Extract MIDI status and data bytes
			PmMessage message = event.message;
			int status = Pm_MessageStatus(message);
			int keyIndex = Pm_MessageData1(message);
			int velocity = Pm_MessageData2(message);

			if ((status == 145 || status == 155) && velocity != 0.0)
				addKeyPressed(keyPressed, keyIndex, velocity);
			else
				removeKeyPressed(keyPressed, keyIndex);
		}

		_midiEvents.clear();
	}
}

void InputManager::addKeyPressed(std::vector<MidiInfo>& keyPressed, int keyIndex, int velocity) const
{
	MidiInfo info = {
		keyIndex,
		velocity,
		true, // rising edge
	};

	removeKeyPressed(keyPressed, keyIndex); // Remove key if it was not released for some reason

	keyPressed.push_back(info);
}

void InputManager::removeKeyPressed(std::vector<MidiInfo>& keyPressed, int keyIndex) const
{
	for (auto it = keyPressed.begin(); it != keyPressed.end(); it++)
	{
		if (it->keyIndex == keyIndex)
		{
			keyPressed.erase(it);
			break;
		}
	}
}

void InputManager::pollMidiDevices(bool log)
{
	if (_midiStream)
	{
		_midiEvents.readNewEvents(_midiStream); // Store event that might have occured before closing midi stream
		Pm_Close(_midiStream);
		_midiStream = nullptr;
	}

	Pm_Terminate();
	Pm_Initialize();
	const int numDevices = Pm_CountDevices();

	_midiDeviceCount = numDevices;
	_detectedDevices.clear();

	for (int i = 0; i < numDevices; i++)
	{
		const PmDeviceInfo* info = Pm_GetDeviceInfo(i);
		assert(info);

		if (info->input == 0) // Only register device whose input == 1
			continue;

		MidiDevice device = { *info, info->name, i };
		_detectedDevices.push_back(device);

		if (device.name == _midiDeviceUsed)
			openMidiDevice(device, false);
	}
}

bool InputManager::openMidiDevice(const MidiDevice& device, bool log)
{
	PmError errnum = Pm_OpenInput(&_midiStream, device.index, NULL, 512, NULL, NULL);
	if (errnum != pmNoError)
	{
		if (log)
		{
			Logger::log("PortMidi", Error) << "Failed to use device " << device.name << std::endl;
		}
		_midiDeviceUsed.clear();
		return false;
	}
	if (log)
	{
		Logger::log("InputManager", Info) << "Using midi device: " << device.name << " id "<< device.index << std::endl;
	}
	return true;
}

const std::vector<MidiDevice>& InputManager::getDetectedMidiDevices() const
{
	return _detectedDevices;
}

void InputManager::setMidiDeviceUsed(const std::string& deviceName)
{
	for (const MidiDevice& device : _detectedDevices)
	{
		if (deviceName == device.name)
		{
			if (_midiDeviceUsed == deviceName)
			{
				Logger::log("PortMidi", Warning) << "Device " << device.name << " is already in use" << std::endl;
				return;
			}

			const bool success = openMidiDevice(device, true);
			_midiDeviceUsed = deviceName;
			if (!success)
			{
				_midiDeviceUsed.clear();
				_midiStream = nullptr;
			}
			return;
		}
	}
	Logger::log("InputManager", Error) << "Midi device " << deviceName << " was not found" << std::endl;
}

void InputManager::closeMidiDevice()
{
	if (_midiStream == nullptr)
		return;

	Pm_Close(_midiStream);
	_midiStream = nullptr;
	Logger::log("InputManager", Info) << "Closed midi device: " <<_midiDeviceUsed << std::endl;
	_midiDeviceUsed.clear();
}

std::string InputManager::getMidiDeviceUsed() const
{
	return _midiDeviceUsed;
}
