#include "InputManager.hpp"

InputManager::InputManager(GLFWwindow* window)
{
	Pm_Initialize();

	int numDevices = Pm_CountDevices();
	if (numDevices <= 0)
	{
		Logger::log("PortMidi", Warning) << "No MIDI devices found." << std::endl;
		return;
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

	PmError errnum = Pm_OpenInput(&_midiStream, 1, NULL, 512, NULL, NULL);
	if (errnum != pmNoError)
	{
		Logger::log("PortMidi", Error) << Pm_GetErrorText(errnum) << std::endl;
		exit(1);
	}

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
	InputManager* inputManager = static_cast<InputManager*>(glfwGetWindowUserPointer(window));
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

void InputManager::updateKeysState(GLFWwindow* window, const MidiPlayerSettings& settings, std::vector<MidiInfo>& keyPressed)
{
	// Mouse
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);
	cursorDir = ImVec2(xpos - cursorPos.x, ypos - cursorPos.y);
	cursorPos = ImVec2(xpos, ypos);

	unsigned int KbToPianoIndex[12] = { GLFW_KEY_Z, GLFW_KEY_S, GLFW_KEY_X, GLFW_KEY_D, GLFW_KEY_C, GLFW_KEY_V, GLFW_KEY_G, GLFW_KEY_B, GLFW_KEY_H, GLFW_KEY_N, GLFW_KEY_J, GLFW_KEY_M };

	// Get keyboard inputs
	for (int i = GLFW_KEY_SPACE; i < GLFW_KEY_LAST; i++)
		keys[i].updateKeyData((bool)glfwGetKey(window, i));

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
		int numEvents = Pm_Read(_midiStream, buffer, 32);
		for (int i = 0; i < numEvents; i++)
		{
			PmEvent& event = buffer[i];

			// Extract MIDI status and data bytes
			PmMessage message = event.message;
			int status = Pm_MessageStatus(message);
			int keyIndex = Pm_MessageData1(message);
			int velocity = Pm_MessageData2(message);

			Logger::log("KeyInfo", Debug) << "state " << status << " key " << (int)keyIndex << " vel " << (int)velocity << std::endl;
			if ((status == 145 || status == 155) && velocity != 0.0)
				addKeyPressed(keyPressed, keyIndex, velocity);
			else
				removeKeyPressed(keyPressed, keyIndex);
		}
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
