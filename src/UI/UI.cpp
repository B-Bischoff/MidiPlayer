#include "UI/UI.hpp"

UI::UI(GLFWwindow* window, AudioData& audio, const ApplicationPath& path)
	: _imPlot(audio), _path(path)
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

	// Search instruments
	fs::path instrumentDir = _path.ressourceDirectory;
	instrumentDir.append(INSTRUMENTS_DIR);
	for (const auto& file : fs::directory_iterator(instrumentDir))
	{
		if (fs::is_regular_file(file) && file.path().extension() == ".json")
		{
			std::cout << "Found : " << file.path().string() << std::endl;
			_instruments.push_back(file);
		}
	}
}

void UI::update(AudioData& audio, Master& master)
{
	initUpdate();

	ImGui::Begin("Audio Buffer");
	_imPlot.update(audio);
	ImGui::End();

	ImGui::Begin("Node Editor");
	_nodeEditor.update(master);
	ImGui::End();

	ImGui::Begin("Instruments");

	static int selected = -1;


	if (ImGui::Button("Load") && selected != -1)
	{
		_nodeEditor.loadFile(master, _instruments[selected]);
	}

	static char instrumentFilename[128] = "";
	if (ImGui::Button("Save Instrument: "))
	{
		fs::path newInstrumentPath = _path.ressourceDirectory;
		std::string filename = instrumentFilename + std::string(INSTRUMENTS_EXTENSION);
		newInstrumentPath.append(INSTRUMENTS_DIR);
		newInstrumentPath.append(filename);
		_nodeEditor.serialize(newInstrumentPath);

		// Don't add already existing instrument
		if (std::find(std::begin(_instruments), std::end(_instruments), newInstrumentPath) == std::end(_instruments))
			_instruments.push_back(newInstrumentPath);
	}
	ImGui::SameLine();
	ImGui::InputText("filename", instrumentFilename, IM_ARRAYSIZE(instrumentFilename));

	for (int i = 0; i < _instruments.size(); i++)
	{
		char buf[64];
		sprintf(buf, "%s", _instruments[i].filename().string().c_str());
		if (ImGui::Selectable(buf, selected == i))
			selected = i;
	}

	ImGui::End();

	render();
}

void UI::initUpdate()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void UI::render()
{
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
