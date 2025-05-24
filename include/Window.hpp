#pragma once

#define GLEW_STATIC

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <string>

#include "WindowContext.hpp"
#include "Logger.hpp"
#include "imgui.h" // Used for ImVec

class Window {
private:
	GLFWwindow* _window;
	ImVec2 _dimensions;

	static void windowResizeCallback(GLFWwindow* glfwWindow, int width, int height);

public:
	Window(const ImVec2& initialDimensions, const std::string& windowTitle);

	void beginFrame(ImVec4 clearColor = ImVec4(0.1f ,0.1f ,0.1f ,1.0f), int clearedBuffer = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT) const;
	void endFrame() const;

	void setUserPointer(void* data);

	ImVec2 getDimensions() const;
	GLFWwindow* getWindow();
	bool shouldClose() const;
	int getKey(int key) const;
};
