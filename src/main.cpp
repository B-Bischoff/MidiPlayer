#include "imgui.h"
#include "imgui_node_editor.h"
#include "implot.h"
#include <cstring>
#include <unistd.h>

#include "inc.hpp"
#include "envelope.hpp"
#include "nodes.hpp"

static void handleFrameProcessTime(const time_point& startTime, const std::chrono::duration<double>& targetFrameDuration, AudioData& audio);

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

#if defined(IMGUI_IMPL_OPENGL_ES2)
	const char* glsl_version = "#version 100";
#elif defined(__APPLE__)
	const char* glsl_version = "#version 150";
#else
	const char* glsl_version = "#version 130";
#endif
	ImGuiContext* imguiContext = ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);

	ImGui::SetCurrentContext(imguiContext);
	ImPlot::CreateContext();

	ed::EditorContext* nodeEditorContext = ed::CreateEditor();

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

	double t = 0.0;

	float* timeArray = new float[audio.getBufferSize()];
	for (int i = 0; i < audio.getBufferSize(); i++)
		timeArray[i] = 1.0 / (double)audio.sampleRate * i;
	bool copyAudioBuffer = true;
	float* bufferCopy = new float[audio.getBufferSize()];

	bool isFirstFrame = true;
	ImVector<LinkInfo> links;

	std::vector<Node> nodes;
	nodes.push_back(NodeTypeA());
	nodes.push_back(NodeTypeB());

	while (!glfwWindowShouldClose(window) && glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS)
	{
		auto startTime = std::chrono::high_resolution_clock::now();
		auto programElapsedTime = std::chrono::duration_cast<std::chrono::duration<double>>(startTime - audio.startTime);

		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glfwPollEvents();

		handleInput(inputManager, envelopes, t);
		generateAudio(audio, inputManager, envelopes, t);

		/*
		// Lag simulator
		if (++FRAME_COUNT % 180 == 0)
		{
			FRAME_COUNT = 0;
			usleep(1000000);
		}
		*/

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("Test");
		ImGui::Checkbox("Copy audio buffer", &copyAudioBuffer);
		ImPlotFlags f;
		if (ImPlot::BeginPlot("Plot", ImVec2(1600, 800)))
		{
			if (true)
			{
				for (int i = 0; i < 1000; i++)
					memcpy(bufferCopy, audio.buffer, audio.getBufferSize() * sizeof(float));
			}
			ImPlot::PlotLine("line", timeArray, bufferCopy, audio.getBufferSize());
			double writeCursorX = audio.writeCursor / (double)audio.sampleRate;
			double readCursorX = audio.leftPhase / (double)audio.sampleRate;
			double writeCursorXArray[2] = { writeCursorX, writeCursorX };
			double readCursorXArray[2] = { readCursorX, readCursorX };
			double cursorY[2] = { 0.0, 1.0 };
			ImPlot::PlotLine("write cursor", writeCursorXArray, cursorY, 2);
			ImPlot::PlotLine("read cursor", readCursorXArray, cursorY, 2);

			ImPlot::EndPlot();


		}
		ImGui::End();


		ed::SetCurrentEditor(nodeEditorContext);
		ed::Begin("Node editor", ImVec2(0.0, 0.0f));

		static ed::NodeId nodeA_Id = getNextId();
		static ed::PinId nodeA_InputPinId = getNextId();
		static ed::PinId nodeA_OutputPinId = getNextId();

		if (isFirstFrame)
		{
			//Node* test = spawnNode();
			//ed::SetNodePosition(test->id, ImVec2(50, 10));
			ed::SetNodePosition(nodeA_Id, ImVec2(10, 10));
		}
		ed::BeginNode(nodeA_Id);
		{
			ImGui::Text("Node A");
			ed::BeginPin(nodeA_InputPinId, ed::PinKind::Input);
				ImGui::Text("-> In");
			ed::EndPin();
			ImGui::SameLine();
			ed::BeginPin(nodeA_OutputPinId, ed::PinKind::Output);
				ImGui::Text("Out ->");
			ed::EndPin();
		}
		ed::EndNode();

		static ed::NodeId nodeB_Id = getNextId();
		static ed::PinId nodeB_InputPinId = getNextId();
		static ed::PinId nodeB_OutputPinId = getNextId();

		if (isFirstFrame)
			ed::SetNodePosition(nodeB_Id, ImVec2(150, 10));
		ed::BeginNode(nodeB_Id);
		{
			ImGui::Text("Node B");
			ed::BeginPin(nodeB_InputPinId, ed::PinKind::Input);
				ImGui::Text("-> In");
			ed::EndPin();
			ImGui::SameLine();
			ed::BeginPin(nodeB_OutputPinId, ed::PinKind::Output);
				ImGui::Text("Out ->");
			ed::EndPin();
		}
		ed::EndNode();

		for (Node node : nodes)
			node.render();


		for (auto& linkInfo : links)
		{
			ed::Link(linkInfo.Id, linkInfo.InputId, linkInfo.OutputId);
		}

		if (ed::BeginCreate())
		{
			ed::PinId inputPinId, outputPinId;
			if (ed::QueryNewLink(&inputPinId, &outputPinId))
			{
				// Link creation logic

				if (inputPinId && outputPinId) // both are valid, let's accept link
				{
					if (ed::AcceptNewItem())
					{
						// Since we accepted new link, lets add one to our list of links.
						links.push_back({ ed::LinkId(getNextId()), inputPinId, outputPinId });

						// Draw new link.
						ed::Link(links.back().Id, links.back().InputId, links.back().OutputId);
					}

					// You may choose to reject connection between these nodes
					// by calling ed::RejectNewItem(). This will allow editor to give
					// visual feedback by changing link thickness and color.
				}
			}
		}
		ed::EndCreate();

		// Handle deletion action
		if (ed::BeginDelete())
		{
			// There may be many links marked for deletion, let's loop over them.
			ed::LinkId deletedLinkId;
			while (ed::QueryDeletedLink(&deletedLinkId))
			{
				// If you agree that link can be deleted, accept deletion.
				if (ed::AcceptDeletedItem())
				{
					// Then remove link from your data.
					for (auto& link : links)
					{
						if (link.Id == deletedLinkId)
						{
							links.erase(&link);
							break;
						}
					}
				}

				// You may reject link deletion by calling:
				// ed::RejectDeletedItem();
			}
		}
		ed::EndDelete(); // Wrap up deletion action

		ed::End();


		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		isFirstFrame = false;

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
