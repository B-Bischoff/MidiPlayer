#pragma once

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "ImPlotUI.hpp"
#include "NodeEditorUI.hpp"

#include "path.hpp"

class UI {
private:
	ImGuiContext* _context;
	ImPlotUI _imPlot;
	NodeEditorUI _nodeEditor;
	const ApplicationPath& _path;

	std::vector<fs::path> _instruments;

public:
	UI(GLFWwindow* window, AudioData& audio, const ApplicationPath& path);
	void update(AudioData& audio, Master& master);
	void render();

private:
	void initUpdate();
};
