#pragma once

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "ImPlotUI.hpp"
#include "NodeEditorUI.hpp"
#include "AudioSpectrum.hpp"

#include "AudioBackend/Instrument.hpp"

#include <map>

#include "path.hpp"
#include "Message.hpp"

#include "UI/Log.hpp"

struct WindowsState {
	bool showLog;
	bool showSettings;
};

class UI {
private:
	ImGuiContext* _context;
	ImPlotUI _imPlot;
	NodeEditorUI _nodeEditor;
	AudioSpectrum _audioSpectrum;
	const ApplicationPath& _path;

	Log _log;

	WindowsState _windowsState;

	std::map<std::string, fs::path> _instruments;

	std::queue<Message> _messages;

	// [TODO] key should not use instruments name as a single instrument can be loaded multiple times
	std::map<std::string, std::stringstream> _loadedInstrumentCache;

	Instrument* _selectedInstrument;

public:
	UI(GLFWwindow* window, AudioData& audio, const ApplicationPath& path);
	void update(AudioData& audio, std::vector<Instrument>& instruments, MidiPlayerSettings& settings);
	void render();

private:
	void initUpdate(const int& WIN_WIDTH, const int& WIN_HEIGHT);
	void endUpdate();

	void switchSelectedInstrument(Instrument& newInstrument);

	void updateMenuBar();
	void updateSavedInstruments(std::vector<Instrument>& instruments, std::string& selectedStoredInstrument);
	void updateLoadedInstruments(std::vector<Instrument>& instruments, int& selectedInstrument, bool& loadDefaultInstrument);

	void processEventQueue();
};
