#pragma once

#define IMGUI_DEFINE_MATH_OPERATORS

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "ImPlotUI.hpp"
#include "NodeEditorUI.hpp"
#include "AudioSpectrum.hpp"
#include "ImGuiNotify.hpp"
#include "FileBrowser.hpp"
#include "fa-solid-900.h"

#include "AudioBackend/Instrument.hpp"

#include <map>

#include "Audio.hpp"
#include "Window.hpp"
#include "InputManager.hpp"
#include "path.hpp"
#include "Message.hpp"

#include "UI/Colors.hpp"
#include "UI/Log.hpp"

#include "UI/fonts/RobotoRegular.h"

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
	FileBrowser _fileBrowser;
	const ApplicationPath& _path;

	ImFont* _font;

	Log _log;

	WindowsState _windowsState;

	std::map<std::string, fs::path> _instruments;

	// [TODO] key should not use instruments name as a single instrument can be loaded multiple times
	std::map<std::string, std::stringstream> _loadedInstrumentCache;

	Instrument* _selectedInstrument;

public:
	UI(GLFWwindow* window, Audio& audio, const ApplicationPath& path);
	void update(Window& window, Audio& audio, std::vector<Instrument>& instruments, MidiPlayerSettings& settings, std::queue<Message>& messageQueue, InputManager& inputManager);
	void render();

private:
	void initStyle();
	void initColors();
	void initFonts();

	void initUpdate(const int& WIN_WIDTH, const int& WIN_HEIGHT);
	void endUpdate();

	void createNewInstrument(std::vector<Instrument>& instruments);
	void switchSelectedInstrument(Instrument& newInstrument);

	void updateMenuBar();

	void updateSavedInstruments(std::vector<Instrument>& instruments, std::string& selectedStoredInstrument);
	void updateOverwritePopup(bool& saveInstrument);
	void serializeInstrument(const fs::path& instrumentPath);

	void updateLoadedInstruments(std::vector<Instrument>& instruments, int& selectedInstrument, bool& loadDefaultInstrument);
	void updateSettings(Audio& audio, InputManager& inputManager, MidiPlayerSettings& settings, std::queue<Message>& messages);

	void processEventQueue(std::queue<Message>& messageQueue);

	void helpMarker(const std::string& message);

	// updateSettings helpers
	void updateAudioOutput(Audio& audio);
	void updateAudioSampleRate(Audio& audio, std::queue<Message>& messageQueue);
	void updateAudioChannels(Audio& audio, std::queue<Message>& messageQueue);
	void updateAudioLatency(Audio& audio);
	void updateMuteAudio(Audio& audio);
	void updateMidiSettings(InputManager& inputManager, MidiPlayerSettings& settings);
	void updateUISettings(MidiPlayerSettings& settings);
};
