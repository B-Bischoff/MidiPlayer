#include "UI/UI.hpp"
#include "imgui.h"

UI::UI(GLFWwindow* window, AudioData& audio, const ApplicationPath& path)
	: _imPlot(audio), _audioSpectrum(audio.getFramesPerUpdate(), 4096), _path(path), _selectedInstrument(nullptr)
{
#if defined(IMGUI_IMPL_OPENGL_ES2)
	const char* glsl_version = "#version 100";
#elif defined(__APPLE__)
	const char* glsl_version = "#version 150";
#else
	const char* glsl_version = "#version 130";
#endif
	ImGuiContext* imguiContext = ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
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
#if VERBOSE
			std::cout << "Found : " << file.path().string() << std::endl;
#endif
			assert(!file.path().stem().empty() && "[UI Load Presets] filename must not be empty");
			const std::string presetName = file.path().stem().string();
			_instruments[presetName] = file;
		}
	}

	// Windows state init
	_windowsState.showLog = false;
	_windowsState.showSettings = false;
}

void UI::update(AudioData& audio, std::vector<Instrument>& instruments, MidiPlayerSettings& settings)
{
	initUpdate(1920, 1080);

	updateMenuBar();
	processEventQueue();

	static bool loadDefaultInstrument = false;
	static int selectedInstrument = -1;
	static std::string selectedStoredInstrument = "";

	// [TODO] find a better way to do this
	// Nodes need to be rendered a first time before you can set its position
	// (which is done when loading nodes from file/stream)
	if (loadDefaultInstrument)
	{
		loadDefaultInstrument = false;
		_nodeEditor.loadFile(_selectedInstrument->master, _instruments["default"]);
	}

	updateSavedInstruments(instruments, selectedStoredInstrument);
	updateLoadedInstruments(instruments, selectedInstrument, loadDefaultInstrument);

	_nodeEditor.update(_selectedInstrument->master, _messages, _selectedInstrument);
	_imPlot.update(audio, _messages);
	_audioSpectrum.update(audio);

	if (_windowsState.showLog)
		_log.draw("Log", &_windowsState.showLog);

	if (_windowsState.showSettings)
	{
		ImGui::Begin("Settings", &_windowsState.showSettings);

		ImGui::Checkbox("Use keyboard as MIDI input", &settings.useKeyboardAsInput);
		ImGui::End();
	}

	endUpdate();
}

void UI::updateMenuBar()
{
	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Exit"))
			{
				std::cerr << "Unfortunately, exiting the application is not a feature yet :/" << std::endl;
			}

			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("View"))
		{
			if (ImGui::MenuItem("Log", NULL, _windowsState.showLog))
				_windowsState.showLog = !_windowsState.showLog;
			if (ImGui::MenuItem("Settings", NULL, _windowsState.showSettings))
				_windowsState.showSettings = !_windowsState.showSettings;

			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}
}

void UI::updateSavedInstruments(std::vector<Instrument>& instruments, std::string& selectedStoredInstrument)
{
	ImGui::Begin("Stored instruments");
	ImGui::Text("Save new instrument");
	ImGui::Separator();
	static char instrumentFilename[128] = "";
	if (ImGui::Button("Save Instrument: ") && instruments.size())
	{
		fs::path newInstrumentPath = _path.ressourceDirectory;
		std::string filename = instrumentFilename + std::string(INSTRUMENTS_EXTENSION);
		newInstrumentPath.append(INSTRUMENTS_DIR);
		newInstrumentPath.append(filename);
		_nodeEditor.serialize(newInstrumentPath);

		// Don't add already existing instrument
		if (std::find_if(_instruments.begin(), _instruments.end(), \
					[&newInstrumentPath](const auto& pair) { return pair.second == newInstrumentPath; }) == _instruments.end())
		{
			const std::string presetName = newInstrumentPath.stem().string();
			_instruments[presetName] = newInstrumentPath;
		}
	}
	ImGui::InputText("filename", instrumentFilename, IM_ARRAYSIZE(instrumentFilename));
	ImGui::Dummy(ImVec2(0, 20.0f));

	ImGui::Text("Stored instruments");
	ImGui::Separator();
	if (ImGui::Button("Load instrument") && !selectedStoredInstrument.empty() && _selectedInstrument != nullptr)
		_nodeEditor.loadFile(_selectedInstrument->master, _instruments.at(selectedStoredInstrument));
	for (auto it = _instruments.begin(); it != _instruments.end(); it++)
	{
		char buf[64] = {};
		sprintf(buf, "%s", it->first.c_str());

		if (ImGui::Selectable(buf,selectedStoredInstrument == it->first))
			selectedStoredInstrument = it->first;
	}
	ImGui::End();
}

void UI::updateLoadedInstruments(std::vector<Instrument>& instruments, int& selectedInstrument, bool& loadDefaultInstrument)
{
	ImGui::Begin("Loaded instruments");
	ImGui::Text("Loaded instruments");
	ImGui::Separator();
	if (ImGui::Button("Create new instrument"))
	{
		static int count = 0;
		instruments.push_back(Instrument());
		instruments.back().name = "instrument" + std::to_string(count++);
		if (instruments.size() == 1) // Auto-select new instrument if no other existing
		{
			switchSelectedInstrument(instruments.back());
			selectedInstrument = 0;
			loadDefaultInstrument = true;
		}
		// Update selected instrument ptr in case of ptr invalidation
		_selectedInstrument = &instruments[selectedInstrument];
	}
	for (int i = 0; i < instruments.size(); i++)
	{
		char buf[64];
		sprintf(buf, "%s", instruments[i].name.c_str());
		if (ImGui::Selectable(buf, selectedInstrument == i))
		{
			switchSelectedInstrument(instruments[i]);
			selectedInstrument = i;

			// Load new instrument cache (if any)
			try
			{
				std::stringstream& stream = _loadedInstrumentCache.at(_selectedInstrument->name);
				_nodeEditor.loadFile(_selectedInstrument->master, stream);
			}
			catch (std::out_of_range& e)
			{
				_nodeEditor.loadFile(_selectedInstrument->master, _instruments["default"]);
				Logger::log("NodeEditor", Debug) << "No cache available, loading default" << std::endl;
			}
		}
	}
	ImGui::End();
}

void UI::processEventQueue()
{
	while (!_messages.empty())
	{
		Message& message = _messages.front();
		switch(message.id)
		{
			case UI_SHOW_ADSR_EDITOR : {
				MessageNodeIdAndControlPoints* payload = (MessageNodeIdAndControlPoints*)message.data;
				_imPlot.setPrintEnvelopeEditor(true, payload->controlPoints, payload->nodeId);
				delete payload;
				break;
			}
			case UI_NODE_DELETED : {
				unsigned int* nodeId = (unsigned int*)message.data;
				if (_imPlot.getNodeId() == *nodeId)
					_imPlot.setPrintEnvelopeEditor(false);
				delete nodeId;
				break;
			}
			case UI_ADSR_MODIFIED : {
				_nodeEditor.updateBackend(_selectedInstrument->master);
				break;
			}
			default: {
				assert(0 && "[UI] invalid message.");
			}

		}
		_messages.pop();
	}
}

void UI::initUpdate(const int& WIN_WIDTH, const int& WIN_HEIGHT)
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImVec2(float(WIN_WIDTH), float(WIN_HEIGHT)));

	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoBringToFrontOnFocus | // Use this window as a host for the menubar and docking
		ImGuiWindowFlags_NoNavFocus |
		ImGuiWindowFlags_NoDocking |
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_MenuBar |
		ImGuiWindowFlags_NoBackground;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

	ImGui::Begin("Main", NULL, windowFlags);
	ImGui::PopStyleVar();

	// create a docking space inside our inner window that lets prevents anything from docking in the central node (so we can see our game content)
	ImGui::DockSpace(ImGui::GetID("Dockspace"), ImVec2(0.0f, 0.0f),  ImGuiDockNodeFlags_PassthruCentralNode);
}

void UI::endUpdate()
{
	ImGui::End(); // End main window
}

void UI::render()
{
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void UI::switchSelectedInstrument(Instrument& newInstrument)
{
	// Save current instrument nodes & links to cache
	if (_selectedInstrument)
	{
		// Clear existing instrument cache
		if (!_loadedInstrumentCache[_selectedInstrument->name].str().empty())
			_loadedInstrumentCache[_selectedInstrument->name].str("");

		_nodeEditor.serialize(_loadedInstrumentCache[_selectedInstrument->name]);
	}

	_selectedInstrument = &newInstrument;
}
