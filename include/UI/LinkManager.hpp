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

	void render();

	template <class Archive>
	void serialize(Archive& archive) {
		for (LinkInfo& link : _links)
			archive(cereal::make_nvp("link", link));
	}

	template <class Archive>
	void load(Archive& archive, IDManager& idManager) {
		try{
			int i = 0;
			while (true)
			{
				archive.startNode();
				int id, inputId, outputId;
				archive(id, inputId, outputId);
				archive.finishNode();

				LinkInfo link = {
					id,
					inputId,
					outputId,
				};
				link.Id = idManager.getID(link.Id.Get());
				assert(link.Id.Get() != INVALID_ID);
				_links.push_back(link);
			}
		}
		catch (std::exception& e)
		{ }
	}

private:
	std::list<LinkInfo> findPinLinks(ed::PinId id, int filter = 0);
	void releaseLinkId(IDManager& idManager, ed::LinkId id);
	void eraseLink(ed::LinkId id);
};
