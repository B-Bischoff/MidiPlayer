#pragma once

#include <imgui_node_editor.h>
#include "audio_backend.hpp"
#include "nodes.hpp"
#include "inc.hpp"
#include <fstream>

// SERIALIZE HELPERS

template<class Archive>
void serialize(Archive& archive, const Node& node)
{
	archive(
		CEREAL_NVP(node.id.Get()),
		CEREAL_NVP(node.name),
		CEREAL_NVP(node.inputs),
		CEREAL_NVP(node.outputs),
		//CEREAL_NVP(node.color),
		CEREAL_NVP(node.type)
	);
}

//template<class Archive>
//void serialize(Archive& archive, const ImColor& color)
//{
//	archive(
//		CEREAL_NVP(color.Value.x),
//		CEREAL_NVP(color.Value.y),
//		CEREAL_NVP(color.Value.z),
//		CEREAL_NVP(color.Value.w)
//	);
//}

template<class Archive>
void serialize(Archive& archive, const Pin& pin)
{
	archive(
		CEREAL_NVP(pin.id.Get()),
		CEREAL_NVP(pin.node->id.Get()),
		CEREAL_NVP(pin.name),
		CEREAL_NVP(pin.kind)
	);
}

template<class Archive>
void serialize(Archive& archive, const LinkInfo& link)
{
	archive(
		CEREAL_NVP(link.Id.Get()),
		CEREAL_NVP(link.InputId.Get()),
		CEREAL_NVP(link.OutputId.Get())
		//CEREAL_NVP(link.Color)
	);
}

// ---------------------

class NodeEditorUI {
private:
	ed::EditorContext* _context;
	std::vector<Node*> _nodes;
	ImVector<LinkInfo> _links;

public:
	NodeEditorUI();

	void update(Master& master);

private:
	void render();

	void handleCreation(Master& master);
	void handleLinkCreation(Master& master);
	void handleNodeCreation();

	void handleDeletion(Master& master);
	void handleLinkDeletion(Master& master);
	void handleNodeDeletion();

	template<typename T>
		Node* addNode();
	void removeNodeAndDependencies(ed::NodeId nodeId);

	void serialize();
};
