#pragma once

#include "imgui.h"
#include "imfilebrowser.h"

#include "UI/Message.hpp"
#include "Logger.hpp"

class FileBrowser {
private:
	ImGui::FileBrowser _browser;
	bool _isOpen = false;
	unsigned int _callerNodeId;

public:
	void update(std::queue<Message>& messages);
	void openFileBrowser(const FileBrowserOpenData& data);
};
