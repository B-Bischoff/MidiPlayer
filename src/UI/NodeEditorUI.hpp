#pragma once

#include <cereal/archives/json.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/base_class.hpp>
#include <cereal/types/memory.hpp>

#include <fstream>
#include <memory>
#include <imgui_node_editor.h>
#include "audio_backend.hpp"
#include "nodes.hpp"
#include "inc.hpp"



// SERIALIZE HELPERS

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
void serialize(Archive& archive, Pin& pin)
{
	archive(
		cereal::make_nvp("pin_id", pin.id.Get()),
		cereal::make_nvp("node_id", pin.node->id.Get()),
		cereal::make_nvp("pin_name", pin.name),
		cereal::make_nvp("pin_kind", (int)pin.kind)
	);
}

template<class Archive>
void serialize(Archive& archive, LinkInfo& link)
{
	archive(
		cereal::make_nvp("link_id", link.Id.Get()),
		cereal::make_nvp("link_input_id", link.InputId.Get()),
		cereal::make_nvp("link_output_id", link.OutputId.Get())
		//CEREAL_NVP(link.Color)
	);
}

// ---------------------

class NodeEditorUI {
private:
	ed::EditorContext* _context;
	//std::vector<Node*> _nodes;
	std::vector<std::shared_ptr<Node>> _nodes;
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
