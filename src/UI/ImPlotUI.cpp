#include "UI/ImPlotUI.hpp"

ImPlotUI::ImPlotUI(AudioData& audio)
{
	ImPlot::CreateContext();

	_timeArray = new float[audio.getBufferSize()];
	for (int i = 0; i < audio.getBufferSize(); i++)
		_timeArray[i] = 1.0 / (double)audio.sampleRate * i;

	// Invert the mouse buttons for plot navigation
	ImPlotInputMap& map = ImPlot::GetInputMap();
	map.Pan = ImGuiMouseButton_Right;
	map.Select = ImGuiMouseButton_Middle;
}

void ImPlotUI::update(AudioData& audio)
{
	ImPlotFlags f;
	if (ImPlot::BeginPlot("Plot", ImVec2(1600, 800)))
	{
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
}
