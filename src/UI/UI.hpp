#pragma once

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "ImPlotUI.hpp"
#include "NodeEditorUI.hpp"

class UI {
private:
	ImGuiContext* _context;
	ImPlotUI _imPlot;
	NodeEditorUI _nodeEditor;

public:
	UI(GLFWwindow* window, AudioData& audio);
	void update(AudioData& audio, Master& master);
	void render();

private:
	void initUpdate();
	void endUpdate();
};
