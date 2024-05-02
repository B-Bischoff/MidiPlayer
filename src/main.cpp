#include "imgui.h"
#include "imgui_node_editor.h"
#include "implot.h"
#include <cstring>
#include <unistd.h>

#include "inc.hpp"

#include "UI/UI.hpp"

static void handleFrameProcessTime(const time_point& startTime, const std::chrono::duration<double>& targetFrameDuration, AudioData& audio);

double AudioComponent::time = 0.0;
unsigned int KeyboardFrequency::keyIndex = 0;
//sEnvelopeADSR* ADSR::envelope = nullptr;

GLFWwindow* init(const int WIN_WIDTH, const int WIN_HEIGHT)
{
	// GLFW init
	if (!glfwInit())
	{
		std::cerr << "Failed to initialize GLFW." << std::endl;
		std::cin.get();
		exit(1);
	}

	// Window init
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	GLFWwindow* window = glfwCreateWindow(WIN_WIDTH, WIN_HEIGHT, "2dPlatformer", NULL, NULL);
	if (window == nullptr)
	{
		std::cerr << "Failed to initialize GLFW window." << std::endl;
		std::cin.get();
		glfwTerminate();
		exit(1);
	}
	glfwMakeContextCurrent(window);
	glfwSwapInterval(0);

	// GLEW init
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK)
	{
		std::cerr << "Failed to initialize GLEW." << std::endl;
		std::cin.get();
		glfwTerminate();
		exit(1);
	}

	return window;
}

int main(void)
{
	int FRAME_COUNT = 0;
	const int SCREEN_WIDTH = 1920;
	const int SCREEN_HEIGHT = 1080;
	GLFWwindow* window = init(SCREEN_WIDTH, SCREEN_HEIGHT);

	std::vector<sEnvelopeADSR> envelopes(16);

	AudioData audio = {
		.sampleRate = 44100,
		.channels = 2,
		.bufferDuration = 1,
		.latency = 3,
		.buffer = nullptr,
		.leftPhase = 0, .rightPhase = 1,
		.writeCursor = 0,
		.targetFPS = 60,
	};
	audio.samplesToAdjust = 0;

	const std::chrono::duration<double> targetFrameDuration(1.0f / (double)audio.targetFPS);

	audio.buffer = new float[audio.getBufferSize()];

	if (audio.buffer == nullptr)
		exitError("[ERROR]: ring buffer allocation failed.");

	memset(audio.buffer, 0, sizeof(float) * audio.getBufferSize());

	audio.startTime = std::chrono::high_resolution_clock::now();
	rtAudioInit(audio);

	InputManager inputManager;
	initInput(inputManager);

	std::this_thread::sleep_for(std::chrono::milliseconds(100)); // let rtaudio get more stable [TODO] check if that is necessary
	audio.writeCursor = (audio.leftPhase + audio.getLatencyInFramesPerUpdate()) % audio.getBufferSize();
	//std::cout << "L/R/W : " << audio.leftPhase << " " << audio.rightPhase << " " << audio.writeCursor << std::endl;

	UI ui(window, audio);

	double t = 0.0;

	float* timeArray = new float[audio.getBufferSize()];
	for (int i = 0; i < audio.getBufferSize(); i++)
		timeArray[i] = 1.0 / (double)audio.sampleRate * i;
	bool copyAudioBuffer = true;
	float* bufferCopy = new float[audio.getBufferSize()];

	ImVector<LinkInfo> links;
	Master master;

	std::vector<MidiInfo> keyPressed;

	while (!glfwWindowShouldClose(window) && glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS)
	{
		auto startTime = std::chrono::high_resolution_clock::now();
		auto programElapsedTime = std::chrono::duration_cast<std::chrono::duration<double>>(startTime - audio.startTime);

		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glfwPollEvents();

		handleInput(window, inputManager, envelopes, keyPressed, t);

		generateAudio(audio, master, envelopes, keyPressed, t);

		/*
		// Lag simulator
		if (++FRAME_COUNT % 180 == 0)
		{
			FRAME_COUNT = 0;
			usleep(1000000);
		}
		*/

		ui.update(audio, master);
		ui.render();

		glfwSwapBuffers(window);

		handleFrameProcessTime(startTime, targetFrameDuration, audio);
	}

	// [TODO] should this be in destructor ?
	Pm_Close(inputManager.midiStream);
	Pm_Terminate();
	// [TODO] clean up audio

	ImPlot::DestroyContext();
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	glfwTerminate();
}

static void handleFrameProcessTime(const time_point& startTime, const std::chrono::duration<double>& targetFrameDuration, AudioData& audio)
{
	auto endTime = std::chrono::high_resolution_clock::now();
	auto deltaTime = std::chrono::duration_cast<std::chrono::duration<double>>(endTime - startTime);

	auto sleepDuration = targetFrameDuration - deltaTime;

	auto startTimeP = std::chrono::duration_cast<std::chrono::duration<double>>(startTime - audio.startTime);
	auto endTimeP = std::chrono::duration_cast<std::chrono::duration<double>>(endTime - audio.startTime);

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

		const double maxAllowedLag = (1.0 / audio.targetFPS) * audio.latency;
		if (deltaTime.count() - (1.0 / audio.targetFPS) > maxAllowedLag)
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
