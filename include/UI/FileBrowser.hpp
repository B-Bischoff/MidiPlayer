#pragma once

#include "imgui.h"
#include "imfilebrowser.h"

#include "Logger.hpp"
#include "UI/Message.hpp"

class FileBrowser {
private:
	ImGui::FileBrowser _browser;
	bool _isOpen;

public:
	FileBrowser();

	void update(std::queue<Message>& messages);
	void openFileBrowser();
};
