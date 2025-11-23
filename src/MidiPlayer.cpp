#include "MidiPlayer.hpp"

double AudioComponent::time = 0.0;
unsigned int AudioComponent::nextId = 1;

MidiPlayer::MidiPlayer(const char* executableName, unsigned int windowWidth, unsigned int windowHeight)
	: _midiPollingTimer(1.0)
{
	std::srand(std::time(0));

	const fs::path applicationPath = fs::canonical(fs::path(executableName));
	Logger::log("Application path",Info) << applicationPath.string() << std::endl;
	const fs::path resourceDirectoryPath = findResourcesFolder(applicationPath);
	if (resourceDirectoryPath.string().empty())
		exit(1);

	_applicationPath = {
		.application = applicationPath,
		.resourceDirectory = resourceDirectoryPath,
	};

	_targetFrameDuration = std::chrono::duration<double>(1.0f / (double)_audio.getTargetFPS());

	//_window = std::make_unique<Window>(ImVec2(windowWidth, windowHeight), "MidiPlayer", _applicationPath.resourceDirectory);
	//_inputManager = std::make_unique<InputManager>(_window->getWindow());
	//_windowContext.window = _window.get();
	//_windowContext.inputManager = _inputManager.get();
	//_window->setUserPointer((void*)&_windowContext);

	//_ui = std::make_unique<UI>(_window->getWindow(), _audio, _applicationPath);
	//Logger::subscribeStream(Log::getStream()); // Duplicate all logs to UI
	Logger::log("MidiPlayer", Info) << "Initialization complete." << std::endl;
}

void MidiPlayer::update()
{
	_instruments.push_back(Instrument());
	_instruments.back().name = "default_instrument";

	Instrument& instrument = _instruments.back();
	Oscillator* osc = new Oscillator();
	osc->type = OscType::Sine;
	instrument.master.addInput(0, osc);
	Number* freq = new Number();
	freq->number = 440.0;
	osc->addInput(0, freq);

	bool shouldClose = false;
	while (!shouldClose)
	{
		auto startTime = std::chrono::high_resolution_clock::now();
		const std::chrono::duration<double> deltaTime = startTime - _lastFrameTime;

		//if (_midiPollingTimer.update(deltaTime.count()))
		//	_inputManager->pollMidiDevices(true);

		//_inputManager->updateKeysState(_settings, _keyPressed);
		//_inputManager->createKeysEvents(_messageQueue);

		_audio.update(_instruments, _keyPressed);

		//_ui->update(*_window, _audio, _instruments, _settings, _messageQueue, *_inputManager);
		//_ui->render();

		handleFrameProcessTime(startTime);
		_lastFrameTime = startTime;
	}
}

void MidiPlayer::handleFrameProcessTime(const time_point& startTime)
{
	auto endTime = std::chrono::high_resolution_clock::now();
	auto deltaTime = std::chrono::duration_cast<std::chrono::duration<double>>(endTime - startTime);

	auto sleepDuration = _targetFrameDuration - deltaTime;

	if (sleepDuration > std::chrono::duration<double>(0.0))
	{
		std::this_thread::sleep_for(sleepDuration * 0.9f);
		endTime = std::chrono::high_resolution_clock::now();
		while (endTime - startTime < _targetFrameDuration)
			endTime = std::chrono::high_resolution_clock::now();
	}
	else
	{
		const double maxAllowedLag = (1.0 / _audio.getTargetFPS()) * _audio.getLatency();
		if (deltaTime.count() - (1.0 / _audio.getTargetFPS()) > maxAllowedLag)
		{
			Logger::log("Audio", Warning) << "lag exceeded cursors safety gap" << std::endl;
		}
	}
}

fs::path MidiPlayer::findResourcesFolder(const fs::path& applicationPath, bool verbose)
{
	const std::string resourceFolder = "resources";
	fs::path path = applicationPath;
	path = path.parent_path(); // Remove application name at end of path

	path.append(resourceFolder);
	if (verbose) Logger::log("Resources", Debug) << "trying path: " << path.string() << std::endl;
	while (!fs::exists(path))
	{
		path = path.parent_path(); // Remove resources folder name
		if (path == path.root_path())
		{
			Logger::log("Resources", Error) << "Could not find resources directory." << std::endl;
			return fs::path();
		}

		path = path.parent_path(); // Go up one directory
		path.append(resourceFolder);
		if (verbose) Logger::log("Resources", Debug) << "trying path: " << path.string() << std::endl;
	}
	Logger::log("Resources", Info) << "Found resources at: " << path.string() << std::endl;
	return path;
}
