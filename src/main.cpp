#include "implot.h"
#include "inc.hpp"
#include "envelope.hpp"

static void handleFrameProcessTime(const time_point& startTime, const std::chrono::duration<double>& targetFrameDuration);

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
	const int SCREEN_WIDTH = 1920;
	const int SCREEN_HEIGHT = 1080;
	GLFWwindow* window = init(SCREEN_WIDTH, SCREEN_HEIGHT);

#if defined(IMGUI_IMPL_OPENGL_ES2)
	const char* glsl_version = "#version 100";
#elif defined(__APPLE__)
	const char* glsl_version = "#version 150";
#else
	const char* glsl_version = "#version 130";
#endif
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);

	ImPlot::CreateContext();

	std::vector<sEnvelopeADSR> envelopes(16);

	AudioData audio = {
		.sampleRate = 44100,
		.channels = 1,
		.bufferDuration = 2,
		.latency = 2,
		.buffer = nullptr,
		.leftPhase = 0, .rightPhase = 1,
		.writeCursor = 0,
		.targetFPS = 64,
	};

	const std::chrono::duration<double> targetFrameDuration(1.0f / (double)audio.targetFPS);

	audio.buffer = new float[(int)(audio.sampleRate * audio.bufferDuration) * audio.channels];

	if (audio.buffer == nullptr)
		exitError("[ERROR]: ring buffer allocation failed.");

	memset(audio.buffer, 0, sizeof(float) * audio.getBufferSize());

	rtAudioInit(audio);

	InputManager inputManager;
	initInput(inputManager);

	audio.startTime = std::chrono::high_resolution_clock::now();

	std::this_thread::sleep_for(std::chrono::milliseconds(100)); // let rtaudio get more stable [TODO] check if that is necessary
	audio.writeCursor = audio.leftPhase + ((double)audio.sampleRate / (double)audio.targetFPS) * audio.latency;
	//std::cout << "L/R/W : " << audio.leftPhase << " " << audio.rightPhase << " " << audio.writeCursor << std::endl;

	double t = 0.0;

	float* timeArray = new float[audio.sampleRate];
	for (int i = 0; i < audio.sampleRate; i++)
		timeArray[i] = 1.0 / (double)audio.sampleRate * i;

	while (!glfwWindowShouldClose(window) && glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS)
	{
		auto startTime = std::chrono::high_resolution_clock::now();
		auto programElapsedTime = std::chrono::duration_cast<std::chrono::duration<double>>(startTime - audio.startTime);


		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glfwPollEvents();

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("Test");
		ImGui::Text("text test");

		if (ImPlot::BeginPlot("Plot"))
		{
			ImPlot::PlotLine("line", timeArray, audio.buffer, audio.sampleRate);
			ImPlot::EndPlot();
		}

		ImGui::End();


		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());


		handleInput(inputManager, envelopes, t);
		generateAudio(audio, inputManager, envelopes, t);
		handleFrameProcessTime(startTime, targetFrameDuration);

		glfwSwapBuffers(window);
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

static void handleFrameProcessTime(const time_point& startTime, const std::chrono::duration<double>& targetFrameDuration)
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
		std::cerr << "[WARNING] : update took longer than expected" << std::endl;
}
