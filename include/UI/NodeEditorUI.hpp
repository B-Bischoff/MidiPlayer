#pragma once

#include <cereal/archives/json.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/base_class.hpp>
#include <cereal/types/memory.hpp>

#include <fstream>
#include <memory>
#include <imgui_node_editor.h>
#include "audio_backend.hpp"
#include "IDManager.hpp"
#include "NodeManager.hpp"
#include "LinkManager.hpp"
#include "UIToBackendAdapter.hpp"
#include "inc.hpp"
#include "Message.hpp"
#include "audio_backend.hpp"
#include "AudioBackend/Components/Components.hpp"
#include <sstream>
#include "MidiMath.hpp"

#include "path.hpp"

// SERIALIZE HELPERS
template<class Archive>
void serialize(Archive& archive, Pin& pin)
{
	archive(
		cereal::make_nvp("pin_id", pin.id),
		cereal::make_nvp("node_id", pin.node->id),
		cereal::make_nvp("pin_name", pin.name),
		cereal::make_nvp("pin_kind", (int)pin.kind),
		cereal::make_nvp("pin_mode", (int)pin.mode)
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

template<class Archive>
void serialize(Archive& archive, ImVec2& v)
{
	archive(
		cereal::make_nvp("x", v.x),
		cereal::make_nvp("y", v.y)
	);
}

template<class Archive>
void serialize(Archive& archive, Vec2& v)
{
	archive(
		cereal::make_nvp("x", v.x),
		cereal::make_nvp("y", v.y)
	);
}

// ---------------------

class NodeEditorUI {
private:
	ed::EditorContext* _context;
	IDManager _idManager;
	NodeManager _nodeManager;
	LinkManager _linkManager;
	bool _UIModified;
	bool _navigateToContent;

public:
	NodeEditorUI();

	void update(Master& master, std::queue<Message>& messages, Instrument* selectedInstrument = nullptr);

	void serialize(const fs::path& path);
	void serialize(std::stringstream& stream);
	void loadFile(Master& master, const fs::path& path);
	void loadFile(Master& master, std::stringstream& stream);

	void updateBackend(Master& master);

private:
	void render(std::queue<Message>& messages);

	void handleCreation(Master& master);
	void handleLinkCreation(Master& master);
	void handleNodeCreation();

	void handleDeletion(Master& master, std::queue<Message>& messages);
	void handleLinkDeletion(Master& master);
	void handleNodeDeletion(std::queue<Message>& master);

	void removeNodeAndDependencies(ed::NodeId nodeId);
	std::vector<std::shared_ptr<Node>>::iterator removeNode(std::vector<std::shared_ptr<Node>>& nodes, Node& nodeToDelete);
	void removeLinkContainingId(ImVector<LinkInfo>& links, std::vector<std::shared_ptr<Node>>& nodes, ed::NodeId id);

	void registerNodeIds(Node& node);
};
