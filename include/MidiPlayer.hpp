#pragma once

#include <vector>
#include <queue>
#include <chrono>
#include <memory>

#include "InputManager.hpp"
#include "inc.hpp"
#include "Audio.hpp"
#include "UI/UI.hpp"

#include "Window.hpp"

class MidiPlayer {
public:
	MidiPlayer(const char* executableName, unsigned int windowWidth, unsigned int windowHeight);
	~MidiPlayer();

	void update();

private:
	Audio _audio;
	std::unique_ptr<Window> _window;
	WindowContext _windowContext;
	std::unique_ptr<InputManager> _inputManager;
	std::unique_ptr<UI> _ui;
	MidiPlayerSettings _settings;
	ApplicationPath _applicationPath;

	std::vector<MidiInfo> _keyPressed = {};
	std::vector<Instrument> _instruments = {};
	std::queue<Message> _messageQueue = {};

	Timer _midiPollingTimer;
	time_point _lastFrameTime = {};
	std::chrono::duration<double> _targetFrameDuration;

	void handleFrameProcessTime(const time_point& startTime);
	fs::path findResourcesFolder(const fs::path& applicationPath, bool verbose = false);
};
