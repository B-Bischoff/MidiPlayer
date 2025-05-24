#include "InputManager.hpp"

InputManager::InputManager(GLFWwindow* window)
	: _window(window), _midiEvents(255), _midiDeviceUsed(""), _midiStream(nullptr), _midiDeviceCount(0)
{
	if (_window == nullptr)
	{
		Logger::log("InputManager", Error) << "Invalid window pointer" << std::endl;
		exit(1);
	}

	pollMidiDevices(false);

	// Setup a callback to get mods (ctrl, shift, ...) key state
	// Others key state are obtained using glfwGetKey()
	glfwSetKeyCallback(window, glfwKeyCallback);
}

InputManager::~InputManager()
{
	Pm_Close(_midiStream);
	Pm_Terminate();
}

void InputManager::glfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	WindowContext* userPointer = static_cast<WindowContext*>(glfwGetWindowUserPointer(window));
	if (!userPointer)
	{
		Logger::log("GLFW", Error) << "Could not find window from user pointer" << std::endl;
		exit(1);
	}
	InputManager* inputManager = userPointer->inputManager;
	assert(inputManager);

	constexpr std::array<unsigned int, 6> modKeys = { GLFW_MOD_SHIFT, GLFW_MOD_CONTROL, GLFW_MOD_ALT, GLFW_MOD_SUPER, GLFW_MOD_CAPS_LOCK, GLFW_MOD_NUM_LOCK, };
	for (unsigned int i = 0; i < modKeys.size(); i++)
	{
		const unsigned int keyIndex = modKeys[i];
		inputManager->updateModifierKey(keyIndex, mods & keyIndex);
	}
}

void InputManager::updateModifierKey(unsigned int key, bool pressed)
{
	// Check key index
	if (key < GLFW_MOD_SHIFT || key > GLFW_MOD_NUM_LOCK)
	{
		Logger::log("InputManager", Error) << "Invalid mod key index: " << key << std::endl;
		exit(1);
	}

	keys[key].updateKeyData(pressed);
}

void InputManager::updateKeysState(const MidiPlayerSettings& settings, std::vector<MidiInfo>& keyPressed)
{
	// Mouse
	double xpos, ypos;
	glfwGetCursorPos(_window, &xpos, &ypos);
	cursorDir = ImVec2(xpos - cursorPos.x, ypos - cursorPos.y);
	cursorPos = ImVec2(xpos, ypos);

	unsigned int KbToPianoIndex[12] = { GLFW_KEY_Z, GLFW_KEY_S, GLFW_KEY_X, GLFW_KEY_D, GLFW_KEY_C, GLFW_KEY_V, GLFW_KEY_G, GLFW_KEY_B, GLFW_KEY_H, GLFW_KEY_N, GLFW_KEY_J, GLFW_KEY_M };

	// Get keyboard inputs
	for (int i = GLFW_KEY_SPACE; i < GLFW_KEY_LAST; i++)
		keys[i].updateKeyData((bool)glfwGetKey(_window, i));

	// Reset rising edges
	for (MidiInfo& info : keyPressed)
		info.risingEdge = false;

	if (settings.useKeyboardAsInput)
	{
		// Octaves
		if (keys[GLFW_KEY_O].isDown() && octave > 0)
			octave -= 1;
		if (keys[GLFW_KEY_P].isDown() && octave < maxOctave)
			octave += 1;

		// Notes
		const unsigned int ARRAY_SIZE = sizeof(KbToPianoIndex) / sizeof(KbToPianoIndex[0]);
		for (int i = 0; i < ARRAY_SIZE; i++)
		{
			KeyData& key = keys[KbToPianoIndex[i]];
			int keyIndex = ARRAY_SIZE * octave + i;

			if (key.isDown())
				addKeyPressed(keyPressed, keyIndex, 127);
			else if (key.isUp())
				removeKeyPressed(keyPressed, keyIndex);
		}
	}
	else if (_midiStream != nullptr)
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

void InputManager::createKeysEvents(std::queue<Message>& messageQueue)
{
	if (keys[GLFW_MOD_CONTROL].isPressed() && keys[GLFW_KEY_C].isDown())
		messageQueue.push(MESSAGE_COPY);
	if (keys[GLFW_MOD_CONTROL].isPressed() && keys[GLFW_KEY_V].isDown())
		messageQueue.push(Message(MESSAGE_PASTE, new ImVec2(cursorPos)));
	if (keys[GLFW_MOD_CONTROL].isPressed() && keys[GLFW_KEY_X].isDown())
		messageQueue.push(MESSAGE_CUT);
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

	if (log && numDevices > _midiDeviceCount)
		ImGui::InsertNotification({ImGuiToastType::Info, 5000, "New Midi device detected"});
	else if (log && numDevices < _midiDeviceCount)
		ImGui::InsertNotification({ImGuiToastType::Warning, 5000, "Midi device removed"});

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
			ImGui::InsertNotification({ImGuiToastType::Error, 5000, "Failed to use device %s", device.name.c_str()});
		}
		_midiDeviceUsed.clear();
		return false;
	}
	if (log)
	{
		Logger::log("InputManager", Info) << "Using midi device: " << device.name << " id "<< device.index << std::endl;
		ImGui::InsertNotification({ImGuiToastType::Success, 5000, "Using midi device %s", device.name.c_str()});
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
				ImGui::InsertNotification({ImGuiToastType::Warning, 5000, "Device %s is already in use", device.name.c_str()});
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
	ImGui::InsertNotification({ImGuiToastType::Error, 5000, "Midi device %s was not found", deviceName.c_str()});
}

void InputManager::closeMidiDevice()
{
	if (_midiStream == nullptr)
		return;

	Pm_Close(_midiStream);
	_midiStream = nullptr;
	Logger::log("InputManager", Info) << "Closed midi device: " <<_midiDeviceUsed << std::endl;
	ImGui::InsertNotification({ImGuiToastType::Info, 5000, "Closed midi device: %s", _midiDeviceUsed.c_str()});
	_midiDeviceUsed.clear();
}

std::string InputManager::getMidiDeviceUsed() const
{
	return _midiDeviceUsed;
}
