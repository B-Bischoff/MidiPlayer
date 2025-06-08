#include "Window.hpp"

Window::Window(const ImVec2& initialDimensions, const std::string& windowTitle, const fs::path& resourceDirectory)
	: _window(nullptr), _dimensions(initialDimensions)
{
	// GLFW init
	if (!glfwInit())
	{
		Logger::log("GLFW", Error) << "Failed to initialize GLFW." << std::endl;
		exit(1);
	}

	// Window init
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	_window = glfwCreateWindow(initialDimensions.x, initialDimensions.y, windowTitle.c_str(), NULL, NULL);
	if (_window == nullptr)
	{
		Logger::log("GLFW", Error) << "Failed to initialize window." << std::endl;
		glfwTerminate();
		exit(1);
	}
	glfwMakeContextCurrent(_window);
	glfwSwapInterval(0);

	glfwSetWindowSizeCallback(_window, windowResizeCallback);

	// GLEW init
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK)
	{
		Logger::log("GLEW", Error) << "Failed to initialize GLEW." << std::endl;
		glfwTerminate();
		exit(1);
	}

	// Application logo
	const fs::path logoPath(resourceDirectory / "logo" / _logoFilename);
	if (!fs::exists(logoPath))
	{
		Logger::log("GLFW", Warning) << "Application logo not found: " << logoPath.string() << std::endl;
		return;
	}

	GLFWimage icon;
	icon.pixels = stbi_load(logoPath.c_str(), &icon.width, &icon.height, 0, 4);
	if (!icon.pixels)
	{
		Logger::log("GLFW", Warning) << "Failed to load application logo: " << logoPath.string() << std::endl;
		return;
	}

	glfwSetWindowIcon(_window, 1, &icon);
	stbi_image_free(icon.pixels);
}

void Window::windowResizeCallback(GLFWwindow* glfwWindow, int width, int height)
{
	WindowContext* userPointer = static_cast<WindowContext*>(glfwGetWindowUserPointer(glfwWindow));
	if (!userPointer)
	{
		Logger::log("GLFW", Error) << "Could not find window from user pointer" << std::endl;
		exit(1);
	}
	Window* window = userPointer->window;

	window->_dimensions = ImVec2(width, height);
}

void Window::beginFrame(ImVec4 clearColor, int clearedBuffer) const
{
	glClearColor(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
	glClear(clearedBuffer);
	glfwPollEvents();
}

void Window::endFrame() const
{
	glfwSwapBuffers(_window);
}

void Window::setUserPointer(void* data)
{
	glfwSetWindowUserPointer(_window, data); // Retrieve this window instance in glfw callbacks
}

ImVec2 Window::getDimensions() const
{
	return _dimensions;
}

GLFWwindow* Window::getWindow()
{
	return _window;
}

bool Window::shouldClose() const
{
	return glfwWindowShouldClose(_window);
}

int Window::getKey(int key) const
{
	return glfwGetKey(_window, key);
}
