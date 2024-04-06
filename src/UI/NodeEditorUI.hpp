#pragma once

#include <imgui_node_editor.h>
#include "audio_backend.hpp"
#include "nodes.hpp"
#include "inc.hpp"

class NodeEditorUI {
private:
	ed::EditorContext* _context;
	std::vector<Node*> _nodes;
	ImVector<LinkInfo> _links;

public:
	NodeEditorUI();

	void update(Master& master);
};
