#pragma once

#include <vector>
#include <queue>
#include <chrono>
#include <memory>

#include "InputManager.hpp"
#include "inc.hpp"
#include "Audio.hpp"
#include "path.hpp"

#include "AudioBackend/Components/Components.hpp"

class MidiPlayer {
public:
	MidiPlayer(const char* executableName, unsigned int windowWidth, unsigned int windowHeight);

	void update();

private:
	Audio _audio;
	std::unique_ptr<InputManager> _inputManager;
	MidiPlayerSettings _settings;
	ApplicationPath _applicationPath;

	std::vector<MidiInfo> _keyPressed = {};
	std::vector<Instrument> _instruments = {};

	Timer _midiPollingTimer;
	time_point _lastFrameTime = {};
	std::chrono::duration<double> _targetFrameDuration;

	void handleFrameProcessTime(const time_point& startTime);
	fs::path findResourcesFolder(const fs::path& applicationPath, bool verbose = false);
};
