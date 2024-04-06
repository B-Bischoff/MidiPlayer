#include "UI.hpp"

UI::UI(GLFWwindow* window, AudioData& audio)
	: _imPlot(audio)
{
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
}

void UI::update(AudioData& audio, Master& master)
{
	initUpdate();

	ImGui::Begin("Test");
	_imPlot.update(audio);
	endUpdate();

	// Needs to be done outside of ImGui render scope
	_nodeEditor.update(master);
}

void UI::initUpdate()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void UI::endUpdate()
{
	ImGui::End();
}

void UI::render()
{
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
