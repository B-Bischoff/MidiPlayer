#include "UI/ImPlotUI.hpp"

ImPlotUI::ImPlotUI(AudioData& audio)
{
	_context = ImPlot::CreateContext();

	_timeArray = new float[audio.getBufferSize()];
	for (int i = 0; i < audio.getBufferSize(); i++)
		_timeArray[i] = 1.0 / (double)audio.sampleRate * i;

	// Invert the mouse buttons for plot navigation
	ImPlotInputMap& map = ImPlot::GetInputMap();
	map.Pan = ImGuiMouseButton_Right;
	map.Select = ImGuiMouseButton_Middle;

	initStyle();
}

void ImPlotUI::initStyle()
{
	// Colors
	ImPlot::GetStyle().Colors[ImPlotCol_FrameBg] = UI_Colors::background_mid;

	// Colormap creation
	// Copy the color of the available pastel colormap but add a bit of value to all colors
	const int size = ImPlot::GetColormapSize(ImPlotColormap_Pastel);
	_colormapColors = std::make_unique<ImVec4[]>(size);

	constexpr double saturationOffset = 0.15;
	for (int i = 0; i < size; i++)
	{
		ImVec4 color = ImPlot::GetColormapColor(i, ImPlotColormap_Pastel);
		float h, s, v;
		ImGui::ColorConvertRGBtoHSV(color.x, color.y, color.z, h, s, v);
		ImVec4 saturatedColor;
		ImGui::ColorConvertHSVtoRGB(h, s + saturationOffset, v, saturatedColor.x, saturatedColor.y, saturatedColor.z);
		saturatedColor.w = color.w; // Keep original alpha
		_colormapColors[i] = saturatedColor;
	}
	_colormap = ImPlot::AddColormap("CustomColormap", _colormapColors.get(), size);
	ImPlot::GetStyle().Colormap = _colormap;
}

ImPlotUI::~ImPlotUI()
{
	ImPlot::DestroyContext(_context);
	delete [] _timeArray;
}

void ImPlotUI::update(AudioData& audio, std::queue<Message>& messages)
{
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

	const bool windowOpened = ImGui::Begin("Audio buffer");
	ImGui::PopStyleVar(1);

	if (windowOpened && ImPlot::BeginPlot("Plot", ImVec2(ImGui::GetContentRegionAvail())))
	{
		ImPlot::SetupAxisLimits(ImAxis_Y1, -1.0, 1.0); // Set Y axis go from -1 to +1
		ImPlot::PlotLine("line", _timeArray, audio.buffer, audio.getBufferSize());
		double writeCursorX = audio.writeCursor / (double)audio.sampleRate;
		double readCursorX = audio.leftPhase / (double)audio.sampleRate;
		double writeCursorXArray[2] = { writeCursorX, writeCursorX };
		double readCursorXArray[2] = { readCursorX, readCursorX };
		double cursorY[2] = { 0.0, 1.0 };
		ImPlot::PlotLine("write cursor", writeCursorXArray, cursorY, 2);
		ImPlot::PlotLine("read cursor", readCursorXArray, cursorY, 2);

		ImPlot::EndPlot();
	}

	if (_printEnvelopeEditor)
		printEnvelopeEditor(messages);

	ImGui::End();
}

void ImPlotUI::handleControlPoints(std::queue<Message>& messages)
{
	// 0 : static | 1 : ctrl | 2 : hovered
	unsigned short lookup[8] = { 0, 1, 0, 1, 0, 0, 1, 0};
	const ImVec4 pointsColor[3] = {
		ImPlot::GetColormapColor(0),
		ImPlot::GetColormapColor(2),
		{1, 1, 1, 1},
	};

	bool adsrChanged = false;

	for (int i = 1; i < 8; ++i)
	{
		ImVec2 pos = ImPlot::PlotToPixels(_controlPoints[i].x, _controlPoints[i].y);

		ImGui::SetCursorScreenPos({pos.x - 5, pos.y - 5});
		ImGui::InvisibleButton(("point" + std::to_string(i)).c_str(), ImVec2(10, 10));

		if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
		{
			ImPlotPoint mousePos = ImPlot::PixelsToPlot(ImGui::GetIO().MousePos);

			// Points x pos must be between the previous and next points
			double xMax = (i == 7) ? 10 : _controlPoints[i + 1].x;
			double xMin = (i == 0) ? 0 : _controlPoints[i - 1].x;
			_controlPoints[i].x = std::clamp(mousePos.x, xMin, xMax);
			_controlPoints[i].y = std::clamp(mousePos.y, 0.0, 2.0);

			// Sustain _control points must have the same Y value
			if (i == 4) _controlPoints[5].y = _controlPoints[4].y;
			else if (i == 5) _controlPoints[4].y = _controlPoints[5].y;

			adsrChanged = true;
		}

		if (ImGui::IsItemActive() || ImGui::IsItemHovered())
		{
			lookup[i] = 2;
			ImGui::SetTooltip("(%0.2fs, %0.2f)", _controlPoints[i].x, _controlPoints[i].y);
		}

		ImPlot::PushStyleColor(ImPlotCol_Line, pointsColor[lookup[i]]);
		ImPlot::PlotScatter("##ControlPoints", &_controlPoints[i].x, &_controlPoints[i].y, 1);
		ImPlot::PopStyleColor();
	}

	if (adsrChanged)
		messages.push(Message(UI_ADSR_MODIFIED));
}

void ImPlotUI::printEnvelopeEditor(std::queue<Message>& messages)
{
	bool windowOpen = true;
	if (ImGui::Begin("ADSR Envelope Editor", &windowOpen))
	{
		if (ImPlot::BeginPlot("##ADSRPlot", ImVec2(ImGui::GetContentRegionAvail())))
		{
			handleControlPoints(messages);
			printEnvelopeEditorPoints();
			ImPlot::EndPlot();
		}
	}
	if (!windowOpen)
		setPrintEnvelopeEditor(false);
	ImGui::End();
}

void ImPlotUI::printEnvelopeEditorPoints()
{
	const int numPoints = 200;
	float X[numPoints];
	float Y[numPoints];

	double xMax = _controlPoints[7].x;

	for (int i = 0; i < numPoints; ++i)
	{
		double t = (double)(i) / (double)(numPoints - 1) * xMax; // numPoints - 1 makes t = 1.0 on last iteration.
		Vec2 p;

		if (t <= _controlPoints[2].x) // Attack
			p = bezierQuadratic(_controlPoints[0], _controlPoints[1], _controlPoints[2], \
					inverseLerp(_controlPoints[0].x, _controlPoints[2].x, t));
		else if (t <= _controlPoints[4].x) // Decay
			p = bezierQuadratic(_controlPoints[2], _controlPoints[3], _controlPoints[4], \
					inverseLerp(_controlPoints[2].x, _controlPoints[4].x, t));
		else if (t <= _controlPoints[5].x) // Sustain
			p = lerp(_controlPoints[4], _controlPoints[5], \
					inverseLerp(_controlPoints[4].x, _controlPoints[5].x, t));
		else // Release
			p = bezierQuadratic(_controlPoints[5], _controlPoints[6], _controlPoints[7], \
					inverseLerp(_controlPoints[5].x, _controlPoints[7].x, t));

		X[i] = p.x;
		Y[i] = p.y;
	}

	ImPlot::PlotLine("ADSR", X, Y, numPoints);
}

void ImPlotUI::setPrintEnvelopeEditor(bool value, Vec2* controlPoints, unsigned int nodeId)
{
	if (value)
	{
		assert(controlPoints && "[ImPlotUI] pass controlPoints when printing envelope editor");
		assert(nodeId != 0 && "[ImPlotUI] invalid nodeId");
		_controlPoints = controlPoints;
		_nodeId = nodeId;
	}
	else
	{
		// Reset control points ptr & nodeId
		_controlPoints = nullptr;
		_nodeId = 0;
	}
	_printEnvelopeEditor = value;
}

unsigned int ImPlotUI::getNodeId() const
{
	return _nodeId;
}
