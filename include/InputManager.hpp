#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <portmidi.h>
#include <imgui.h> // [TODO] used for vec2 struct but please replace this
#include "ImGuiNotify.hpp"

#include <array>
#include <vector>

#include "inc.hpp" // MidiPlayerSettings struct
#include "WindowContext.hpp"
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

struct MidiDevice {
	PmDeviceInfo info;
	std::string name;
	int index;
};

class PortMidiEvents {
public:
	PortMidiEvents(const unsigned int eventCapacity) {
		_events.reserve(eventCapacity);
	}

	const std::vector<PmEvent>& getEvents() const {
		return _events;
	}

	void readNewEvents(PortMidiStream* midiStream) {
		const size_t oldSize = _events.size();
		const size_t freeCapacity = _events.capacity() - oldSize;

		if (freeCapacity == 0)
		{
			Logger::log("PortMidiEvents", Warning) << "Buffer full, cannot read new MIDI events." << std::endl;
			return;
		}

		// Ensure size matches capacity
		if (_events.size() < _events.capacity())
			_events.resize(_events.capacity());

		int numEvents = Pm_Read(midiStream, _events.data() + oldSize, static_cast<int>(_events.capacity() - oldSize));

		if (numEvents < 0) {
			Logger::log("PortMidiEvents", Error) << "Error while reading events: " << Pm_GetErrorText(PmError(numEvents)) << std::endl;

			// Roll back size
			_events.resize(oldSize);
			return;
		}

		// Extend size to include new events
		_events.resize(oldSize + static_cast<size_t>(numEvents));
	}

	void clear() {
		_events.clear();
	}

private:
	std::vector<PmEvent> _events;
};

class InputManager {
private:
	GLFWwindow* _window; // Initialized by the Window class

	PmStream* _midiStream = nullptr;
	PortMidiEvents _midiEvents;

	std::vector<MidiDevice> _detectedDevices;
	int _midiDeviceCount;
	std::string _midiDeviceUsed;

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
	void updateKeysState(const MidiPlayerSettings& settings, std::vector<MidiInfo>& keyPressed);
	void createKeysEvents(std::queue<Message>& messageQueue);

	void pollMidiDevices(bool log = false);

	const std::vector<MidiDevice>& getDetectedMidiDevices() const;
	void setMidiDeviceUsed(const std::string& deviceName);
	std::string getMidiDeviceUsed() const;
	void closeMidiDevice();

private:
	static void glfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

	void addKeyPressed(std::vector<MidiInfo>& keyPressed, int keyIndex, int velocity) const;
	void removeKeyPressed(std::vector<MidiInfo>& keyPressed, int keyIndex) const;

	bool openMidiDevice(const MidiDevice& device, bool log = false);
};
