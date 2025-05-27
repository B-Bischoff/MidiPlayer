#include "imgui.h"
#include "implot.h"
#include <cstring>
#include <filesystem>
#include <unistd.h>

#include "inc.hpp"

#include "UI/UI.hpp"
#include "InputManager.hpp"
#include "AudioBackend/Instrument.hpp"
#include "Audio.hpp"
#include "Window.hpp"

static void handleFrameProcessTime(const time_point& startTime, const std::chrono::duration<double>& targetFrameDuration, Audio& audio);

double AudioComponent::time = 0.0;
unsigned int AudioComponent::nextId = 1;
unsigned int KeyboardFrequency::keyIndex = 0;

fs::path findRessourcesFolder(const fs::path& applicationPath, bool verbose = false)
{
	const std::string ressourceFolder = "ressources";
	fs::path path = applicationPath;
	path = path.parent_path(); // Remove application name at end of path

	path.append(ressourceFolder);
	if (verbose) Logger::log("Ressources", Debug) << "trying path: " << path.string() << std::endl;
	while (!fs::exists(path))
	{
		path = path.parent_path(); // Remove ressources folder name
		if (path == path.root_path())
		{
			Logger::log("Ressources", Error) << "Could not find ressources directory." << std::endl;
			return fs::path();
		}

		path = path.parent_path(); // Go up one directory
		path.append(ressourceFolder);
		if (verbose) Logger::log("Ressources", Debug) << "trying path: " << path.string() << std::endl;
	}
	Logger::log("Ressources", Info) << "Found ressources at: " << path.string() << std::endl;
	return path;
}

int main(int argc, char* argv[])
{
	const fs::path applicationPath = fs::canonical(fs::path(argv[0]));
	Logger::log("Application path",Info) << applicationPath.string() << std::endl;
	const fs::path ressourceDirectoryPath = findRessourcesFolder(applicationPath);
	if (ressourceDirectoryPath.string().empty())
		exit(1);

	std::srand(std::time(0));

	ApplicationPath path = {
		.application = applicationPath,
		.ressourceDirectory = ressourceDirectoryPath,
	};

	int FRAME_COUNT = 0;

	MidiPlayerSettings settings;

	Audio audio;
	constexpr unsigned int SCREEN_WIDTH = 1920;
	constexpr unsigned int SCREEN_HEIGHT = 1080;
	Window window(ImVec2(SCREEN_WIDTH, SCREEN_HEIGHT), "MidiPlayer");

	const std::chrono::duration<double> targetFrameDuration(1.0f / (double)audio.getTargetFPS());

	InputManager inputManager(window.getWindow());
	WindowContext windowContext = { &window, &inputManager };
	window.setUserPointer((void*)&windowContext);

	UI ui(window.getWindow(), audio, path);
	Logger::subscribeStream(Log::getStream()); // Duplicate all logs to UI

	std::vector<Instrument> instruments = {};

	std::vector<MidiInfo> keyPressed = {};

	std::queue<Message> messageQueue = {};

	Timer midiPollingTimer(1.0);
	double refreshCooldown = 0;
	time_point lastFrameTime;
	while (!window.shouldClose() && glfwGetKey(window.getWindow(), GLFW_KEY_ESCAPE) != GLFW_PRESS)
	{
		auto startTime = std::chrono::high_resolution_clock::now();
		const std::chrono::duration<double> deltaTime = startTime - lastFrameTime;

		window.beginFrame(ImVec4(0.1f, 0.1f, 0.1f, 1.0f), GL_COLOR_BUFFER_BIT);

		if (midiPollingTimer.update(deltaTime.count()))
			inputManager.pollMidiDevices(true);

		inputManager.updateKeysState(settings, keyPressed);
		inputManager.createKeysEvents(messageQueue);

		audio.update(instruments, keyPressed);

		/*
		// Lag simulator
		if (++FRAME_COUNT % 180 == 0)
		{
			FRAME_COUNT = 0;
			usleep(1000000);
		}
		*/

		ui.update(window, audio, instruments, settings, messageQueue, inputManager);
		ui.render();

		window.endFrame();

		handleFrameProcessTime(startTime, targetFrameDuration, audio);
		lastFrameTime = startTime;
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	glfwTerminate();
}

static void handleFrameProcessTime(const time_point& startTime, const std::chrono::duration<double>& targetFrameDuration, Audio& audio)
{
	auto endTime = std::chrono::high_resolution_clock::now();
	auto deltaTime = std::chrono::duration_cast<std::chrono::duration<double>>(endTime - startTime);

	auto sleepDuration = targetFrameDuration - deltaTime;

	if (sleepDuration > std::chrono::duration<double>(0.0))
	{
		std::this_thread::sleep_for(sleepDuration * 0.9f);
		endTime = std::chrono::high_resolution_clock::now();
		while (endTime - startTime < targetFrameDuration)
			endTime = std::chrono::high_resolution_clock::now();
	}
	else
	{
		//std::cerr << "[WARNING] : update took longer than expected : " << deltaTime.count() << std::endl;

		const double maxAllowedLag = (1.0 / audio.getTargetFPS()) * audio.getLatency();
		if (deltaTime.count() - (1.0 / audio.getTargetFPS()) > maxAllowedLag)
		{
			// [TODO] handle lag whose duration is greater than theoric latency between read & write cursors
			// Test showed that handle this kind of lag is not mandatory
			// It would be nice to supress harsh sounds but is it even possible?
			std::cerr << "[WARNING] : lag exceeded cursors safety gap" << std::endl;
			/*
			memset(audio.buffer, 0, audio.getBufferSize() * sizeof(float));
			audio.leftPhase = 0;
			audio.rightPhase = 1;
			audio.writeCursor = audio.getLatencyInFramesPerUpdate();
			audio.samplesToAdjust = 0;
			*/
		}
	}
}
