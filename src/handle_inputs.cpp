#include "inc.hpp"
#include "config.hpp"
#include "envelope.hpp"

static void addKeyPressed(std::vector<MidiInfo>& keyPressed, int keyIndex, int velocity);
static void removeKeyPressed(std::vector<MidiInfo>& keyPressed, int keyIndex);

void initInput(InputManager& inputManger) // [TODO] Should this be in a constructor ?
{
	Pm_Initialize();

	int numDevices = Pm_CountDevices();
	if (numDevices <= 0)
	{
		Logger::log("PortMidi", Error) << "No MIDI devices found." << std::endl;
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

	PmError errnum = Pm_OpenInput(&inputManger.midiStream, 1, NULL, 512, NULL, NULL);
	if (errnum != pmNoError)
	{
		Logger::log("PortMidi", Error) << Pm_GetErrorText(errnum) << std::endl;
		exit(1);
	}
}

void updateKeyData(KeyData& key, bool keyPressed)
{
	key.pressed = keyPressed;
	key.down = key.pressed & !key.lastFramePressed;
	key.up = !key.pressed & key.lastFramePressed;
	key.lastFramePressed = key.pressed;
}

void glfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	InputManager* inputManager = static_cast<InputManager*>(glfwGetWindowUserPointer(window));
	assert(inputManager);

	constexpr std::array<unsigned int, 8> modKeys = { GLFW_MOD_SHIFT, GLFW_MOD_CONTROL, GLFW_MOD_ALT, GLFW_MOD_SUPER, GLFW_MOD_CAPS_LOCK, GLFW_MOD_NUM_LOCK, };
	for (unsigned int i = 0; i < modKeys.size(); i++)
	{
		const unsigned int keyIndex = modKeys[i];
		updateKeyData(inputManager->keys[keyIndex], mods & keyIndex);
	}
}

void updateKeysState(GLFWwindow* window, const MidiPlayerSettings& settings, InputManager& inputManager, std::vector<MidiInfo>& keyPressed, double time)
{
	// Mouse
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);
	inputManager.cursorDir = ImVec2(xpos - inputManager.cursorPos.x, ypos - inputManager.cursorPos.y);
	inputManager.cursorPos = ImVec2(xpos, ypos);

	unsigned int KbToPianoIndex[12] = { GLFW_KEY_Z, GLFW_KEY_S, GLFW_KEY_X, GLFW_KEY_D, GLFW_KEY_C, GLFW_KEY_V, GLFW_KEY_G, GLFW_KEY_B, GLFW_KEY_H, GLFW_KEY_N, GLFW_KEY_J, GLFW_KEY_M };

	// Get keyboard inputs
	for (int i = GLFW_KEY_SPACE; i < GLFW_KEY_LAST; i++)
		updateKeyData(inputManager.keys[i], (bool)glfwGetKey(window, i));

	// Reset rising edges
	for (MidiInfo& info : keyPressed)
		info.risingEdge = false;

	if (settings.useKeyboardAsInput)
	{
		// Octaves
		if (inputManager.keys[GLFW_KEY_O].down && inputManager.octave > 0)
			inputManager.octave -= 1;
		if (inputManager.keys[GLFW_KEY_P].down && inputManager.octave < inputManager.maxOctave)
			inputManager.octave += 1;

		// Notes
		const unsigned int ARRAY_SIZE = sizeof(KbToPianoIndex) / sizeof(KbToPianoIndex[0]);
		for (int i = 0; i < ARRAY_SIZE; i++)
		{
			KeyData& key = inputManager.keys[KbToPianoIndex[i]];
			int keyIndex = ARRAY_SIZE * inputManager.octave + i;

			if (key.down)
				addKeyPressed(keyPressed, keyIndex, 127);
			else if (key.up)
				removeKeyPressed(keyPressed, keyIndex);
		}
	}
	else
	{
		int numEvents = Pm_Read(inputManager.midiStream, inputManager.buffer, 32);
		for (int i = 0; i < numEvents; i++)
		{
			PmEvent& event = inputManager.buffer[i];

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

static void addKeyPressed(std::vector<MidiInfo>& keyPressed, int keyIndex, int velocity)
{
	MidiInfo info = {
		keyIndex,
		velocity,
		true, // rising edge
	};

	keyPressed.push_back(info);
}

static void removeKeyPressed(std::vector<MidiInfo>& keyPressed, int keyIndex)
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

void createKeysEvents(InputManager& inputManager, std::queue<Message>& messageQueue)
{
	if (inputManager.keys[GLFW_MOD_CONTROL].pressed && inputManager.keys[GLFW_KEY_C].down)
		messageQueue.push(MESSAGE_COPY);
	if (inputManager.keys[GLFW_MOD_CONTROL].pressed && inputManager.keys[GLFW_KEY_V].down)
		messageQueue.push(Message(MESSAGE_PASTE, new ImVec2(inputManager.cursorPos)));
	if (inputManager.keys[GLFW_MOD_CONTROL].pressed && inputManager.keys[GLFW_KEY_X].down)
		messageQueue.push(MESSAGE_CUT);
}
