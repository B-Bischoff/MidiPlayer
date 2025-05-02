#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <portmidi.h>
#include <imgui.h> // [TODO] used for vec2 struct but please replace this

#include <array>
#include <vector>

#include "inc.hpp" // MidiPlayerSettings struct
#include "Logger.hpp"

// Keyboard
class KeyData {
private:
	bool _down; // rising edge (true for the first frame when key held down)
	bool _up; // falling edge (true one frame when key released)
	bool _pressed; // press state (true if key held down)

	bool _lastFramePressed; // internal value used to update all the above

public:
	void updateKeyData(bool keyPressed)
	{
		_pressed = keyPressed;
		_down = _pressed & !_lastFramePressed;
		_up = !_pressed & _lastFramePressed;
		_lastFramePressed = _pressed;
	}

	bool isDown() const {
		return _down;
	}
	bool isUp() const {
		return _up;
	}
	bool isPressed() const {
		return _pressed;
	}
};

class InputManager {
private:
	// Midi events (PortMidi specifics)
	PmStream* _midiStream = nullptr;
	PmEvent buffer[32];

	// Keyboard data
	KeyData keys[GLFW_KEY_LAST] = {};
	unsigned int octave = 4;
	static constexpr unsigned int maxOctave = 8;

	// Mouse
	static constexpr int GLFW_MAX_MOUSE_BTN = 8;
	KeyData mouseButtons[GLFW_MAX_MOUSE_BTN];
	// [TODO] use glm?
	ImVec2 cursorPos;
	ImVec2 cursorDir;
	// [TODO] add scroll

public:
	InputManager(GLFWwindow* window);
	~InputManager();

	void updateModifierKey(unsigned int key, bool pressed);
	void updateKeysState(GLFWwindow* window, const MidiPlayerSettings& settings, std::vector<MidiInfo>& keyPressed);
	void createKeysEvents(std::queue<Message>& messageQueue);

private:
	static void glfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

	void addKeyPressed(std::vector<MidiInfo>& keyPressed, int keyIndex, int velocity) const;
	void removeKeyPressed(std::vector<MidiInfo>& keyPressed, int keyIndex) const;
};
