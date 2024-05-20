#pragma once

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "ImPlotUI.hpp"
#include "NodeEditorUI.hpp"

#include "AudioBackend/Instrument.hpp"

#include <map>

#include "path.hpp"

class UI {
private:
	ImGuiContext* _context;
	ImPlotUI _imPlot;
	NodeEditorUI _nodeEditor;
	const ApplicationPath& _path;

	std::map<std::string, fs::path> _instruments;

	// [TODO] key should not use instruments name as a single instrument can be loaded multiple times
	std::map<std::string, std::stringstream> _loadedInstrumentCache;

	Instrument* _selectedInstrument;

public:
	UI(GLFWwindow* window, AudioData& audio, const ApplicationPath& path);
	void update(AudioData& audio, std::vector<Instrument>& instruments);
	void render();

private:
	void initUpdate();
	void switchSelectedInstrument(Instrument& newInstrument);
};
