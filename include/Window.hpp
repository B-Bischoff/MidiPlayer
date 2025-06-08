#pragma once

#define GLEW_STATIC

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <string>

#include "stb_image_impl.hpp"
#include "imgui.h" // Used for ImVec

#include "path.hpp"
#include "WindowContext.hpp"
#include "Logger.hpp"

class Window {
private:
	GLFWwindow* _window;
	ImVec2 _dimensions;

	static void windowResizeCallback(GLFWwindow* glfwWindow, int width, int height);

	static constexpr const char* _logoFilename = "midiplayer-logo.png";

public:
	Window(const ImVec2& initialDimensions, const std::string& windowTitle, const fs::path& resourceDirectory);

	void beginFrame(ImVec4 clearColor = ImVec4(0.1f ,0.1f ,0.1f ,1.0f), int clearedBuffer = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT) const;
	void endFrame() const;

	void setUserPointer(void* data);

	ImVec2 getDimensions() const;
	GLFWwindow* getWindow();
	bool shouldClose() const;
	int getKey(int key) const;
};
