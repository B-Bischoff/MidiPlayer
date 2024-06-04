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
// Define control points for ADSR
ImVec2 controlPoints[8] = {
	{0.0f, 0.0f}, // static 0
	{0.1f, 0.4f}, // ctrl 0
	{0.2f, 1.0f}, // static 1
	{0.3f, 0.9f}, // ctrl 1
	{0.5f, 0.8f}, // static 2
	{0.8f, 0.8f}, // static 3
	{0.9f, 0.6f}, // ctrl 2
	{1.0f, 0.0f}, // static 4
};

void HandleControlPoints()
{
	// 0 : static | 1 : ctrl
	bool lookup[8] = { 0, 1, 0, 1, 0,0 , 1, 0};
	ImVec4 pointsColor[2] = {
		{1, 0, 0, 1},
		{0, 1, 0, 1},
	};
	for (int i = 1; i < 8; ++i)
	{
		ImVec2 pos = ImPlot::PlotToPixels(controlPoints[i].x, controlPoints[i].y);

		ImGui::SetCursorScreenPos({pos.x - 5, pos.y - 5});
		ImGui::InvisibleButton(("point" + std::to_string(i)).c_str(), ImVec2(10, 10));

		if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
		{
			ImPlotPoint mousePos = ImPlot::PixelsToPlot(ImGui::GetIO().MousePos);
			double xMax = (i == 7) ? 10 : controlPoints[i + 1].x;
			double xMin = (i == 0) ? 0 : controlPoints[i - 1].x;
			controlPoints[i].x = std::clamp(mousePos.x, xMin, xMax);
			controlPoints[i].y = std::clamp(mousePos.y, 0.0, 2.0);

			// Sustain control points must have the same Y value
			if (i == 4)
				controlPoints[5].y = controlPoints[4].y;
			else if (i == 5)
				controlPoints[4].y = controlPoints[5].y;
		}

		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("(%0.2f, %0.2f)", controlPoints[i].x, controlPoints[i].y);

		ImPlot::PushPlotClipRect();
		ImPlotContext* context = ImPlot::GetCurrentContext();
		ImPlot::PushStyleColor(ImPlotCol_Line, pointsColor[lookup[i]]);
		ImPlot::PlotScatter("##ControlPoints", &controlPoints[i].x, &controlPoints[i].y, 1);
		ImPlot::PopStyleColor();
		ImPlot::PopPlotClipRect();
	}
}

ImVec2 lerp(const ImVec2& p0, const ImVec2& p1, double t)
{
	ImVec2 v;
	v.x = (1.0 - t) * p0.x + t * p1.x;
	v.y = (1.0 - t) * p0.y + t * p1.y;
	return v;
}

double inverseLerp(double start, double end, double value)
{
	return (value - start) / (end - start);
}

ImVec2 bezierQuadratic(const ImVec2& p0, const ImVec2& p1, const ImVec2& p2, double t)
{
	ImVec2 p0p1 = lerp(p0, p1, t);
	ImVec2 p1p2 = lerp(p1, p2, t);
	return lerp(p0p1, p1p2, t);
}

void PlotBezierCurve()
{
	const int numPoints = 200;
	float X[numPoints];
	float Y[numPoints];

	double xMax = controlPoints[7].x;

	for (int i = 0; i < numPoints; ++i)
	{
		double t = (double)(i) / (double)(numPoints - 1) * xMax; // numPoints - 1 makes t = 1.0 on last iteration.
		ImVec2 p;

		if (t <= controlPoints[2].x) // Attack
			p = bezierQuadratic(controlPoints[0], controlPoints[1], controlPoints[2], \
					inverseLerp(controlPoints[0].x, controlPoints[2].x, t));
		else if (t <= controlPoints[4].x) // Decay
			p = bezierQuadratic(controlPoints[2], controlPoints[3], controlPoints[4], \
					inverseLerp(controlPoints[2].x, controlPoints[4].x, t));
		else if (t <= controlPoints[5].x) // Sustain
			p = lerp(controlPoints[4], controlPoints[5], \
					inverseLerp(controlPoints[4].x, controlPoints[5].x, t));
		else // Release
			p = bezierQuadratic(controlPoints[5], controlPoints[6], controlPoints[7], \
					inverseLerp(controlPoints[5].x, controlPoints[7].x, t));

		X[i] = p.x;
		Y[i] = p.y;
	}

	ImPlot::PlotLine("ADSR", X, Y, numPoints);
}

void UI::update(AudioData& audio, std::vector<Instrument>& instruments)
{
	initUpdate();

	static bool showADSREditor = true;
	if (showADSREditor)
	{
		if (ImGui::Begin("ADSR Envelope Editor", &showADSREditor))
		{
			if (ImPlot::BeginPlot("##ADSRPlot", ImVec2(-1, 0)))
			{
				ImPlot::SetupAxes("Time", "Amplitude");
				ImPlot::SetupAxisLimits(ImAxis_X1, 0.0, 1.0);
				ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0, 1.0);

				PlotBezierCurve();
				HandleControlPoints();

				ImPlot::EndPlot();
			}
		}
		ImGui::End();
	}

	ImGui::Begin("Audio Buffer");
	_imPlot.update(audio);
	ImGui::End();

	ImGui::Begin("Node Editor");
	if (_selectedInstrument)
		_nodeEditor.update(_selectedInstrument->master);
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
