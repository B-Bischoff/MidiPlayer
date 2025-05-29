#pragma once

#include <memory>
#include <vector>
#include <list>
#include "nodes.hpp"

#include "NodeManager.hpp"
#include "IDManager.hpp"

class LinkManager {
private:
	std::vector<LinkInfo> _links;

public:
	void addLink(IDManager& idManager, NodeManager& nodeManager, ed::PinId input, ed::PinId output);
	void removeLink(IDManager& idManager, ed::LinkId id);
	void removeAllLinks(IDManager& idManager);
	void removeLinksFromNodeId(IDManager& idManager, NodeManager& nodeManager, ed::NodeId id);
	void removeLinksContainingPinId(IDManager& idManager, ed::PinId id);

	/*
	 * filter is used to select which links are returned:
	 * 0 : link is added no matter if its an input or output.
	 * 1 : link is added to list only if id is an input.
	 * 2 : link is added to list only if id is an output.
	 */
	std::list<LinkInfo> findNodeLinks(NodeManager& nodeManager, ed::NodeId id, int filter = 0);
	std::list<LinkInfo> findPinLinks(ed::PinId id, int filter = 0);

	void render();

	template <class Archive>
	void serialize(Archive& archive);

	template <class Archive>
	void load(Archive& archive, IDManager& idManager);

private:
	void releaseLinkId(IDManager& idManager, ed::LinkId id);
	void eraseLink(ed::LinkId id);
};

// Template method definition

template<class Archive>
void serialize(Archive& archive, Pin& pin)
{
	archive(
		cereal::make_nvp("pin_id", pin.id),
		cereal::make_nvp("node_id", pin.node->id),
		cereal::make_nvp("pin_name", pin.name),
		cereal::make_nvp("pin_kind", (int)pin.kind),
		cereal::make_nvp("pin_input_id", pin.inputId),
		cereal::make_nvp("pin_mode", (int)pin.mode)
	);
}

struct LinkData {
	int id;
	int inputId;
	int outputId;
};

template<class Archive>
void serialize(Archive& archive, LinkData& link)
{
	archive(
		cereal::make_nvp("link_id", link.id),
		cereal::make_nvp("link_input_id", link.inputId),
		cereal::make_nvp("link_output_id", link.outputId)
		//CEREAL_NVP(link.Color)
	);
}

template <class Archive>
void LinkManager::serialize(Archive& archive) {
	std::vector<LinkData> linksData;
	for (const LinkInfo& link : _links)
	{
		LinkData linkData = { (int)link.Id.Get(), (int)link.InputId.Get(), (int)link.OutputId.Get() };
		linksData.push_back(linkData);
	}
	archive(cereal::make_nvp("links", linksData));
}

template <class Archive>
void LinkManager::load(Archive& archive, IDManager& idManager) {
	std::vector<LinkData> links;
	archive(links);

	for (LinkData& link : links)
	{
		link.id = idManager.getID(link.id); assert(link.id != INVALID_ID);
		_links.push_back( {link.id, link.inputId, link.outputId} );
	}
}
