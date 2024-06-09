#include "UI/UI.hpp"

UI::UI(GLFWwindow* window, AudioData& audio, const ApplicationPath& path)
	: _imPlot(audio), _path(path), _selectedInstrument(nullptr)
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
#if VERBOSE
			std::cout << "Found : " << file.path().string() << std::endl;
#endif
			assert(!file.path().stem().empty() && "[UI Load Presets] filename must not be empty");
			const std::string presetName = file.path().stem().string();
			_instruments[presetName] = file;
		}
	}
}

void UI::update(AudioData& audio, std::vector<Instrument>& instruments)
{
	initUpdate();

	// Process message/events queue
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
			default: {
				assert(0 && "[UI] invalid message.");
			}

		}
		_messages.pop();
	}

	_imPlot.update(audio);

	ImGui::Begin("Node Editor");
	if (_selectedInstrument)
		_nodeEditor.update(_selectedInstrument->master, _messages);
	ImGui::End();

	ImGui::Begin("Loaded Instruments");
	static int selectedInstrument = -1;

	// [TODO] find a better way to do this
	// Nodes need to be rendered a first time before you can set its position
	// (which is done when loading nodes from file/stream)
	static bool loadDefaultInstrument = false;
	if (loadDefaultInstrument)
	{
		loadDefaultInstrument = false;
		_nodeEditor.loadFile(_selectedInstrument->master, _instruments["default"]);
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
				std::cout << "No cache available, loading default" << std::endl;
			}
		}
	}

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

	ImGui::End();

	ImGui::Begin("Stored Instruments");

	static std::string selectedStoredInstrument = "";
	if (ImGui::Button("Load") && !selectedStoredInstrument.empty() && _selectedInstrument != nullptr)
		_nodeEditor.loadFile(_selectedInstrument->master, _instruments.at(selectedStoredInstrument));

	static char instrumentFilename[128] = "";
	if (ImGui::Button("Save Instrument: "))
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
	ImGui::SameLine();
	ImGui::InputText("filename", instrumentFilename, IM_ARRAYSIZE(instrumentFilename));

	for (auto it = _instruments.begin(); it != _instruments.end(); it++)
	{
		char buf[64];
		sprintf(buf, "%s", it->first.c_str());
		if (ImGui::Selectable(buf, selectedStoredInstrument == it->first))
			selectedStoredInstrument = it->first;
	}

	ImGui::End();
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
