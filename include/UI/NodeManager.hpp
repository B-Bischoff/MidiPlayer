#pragma once

#include <list>
#include <memory>
#include <vector>

#include "nodes.hpp"
#include "IDManager.hpp"

// [TODO] give NodeManager and LinkManager their own IDManager ??
class NodeManager {
private:
	std::vector<std::shared_ptr<Node>> _nodes;

public:
	template<typename T>
	std::shared_ptr<Node> addNode(IDManager& idManager);
	void removeNode(IDManager& idManager, std::shared_ptr<Node>& node);
	void removeNode(IDManager& idManager, ed::NodeId id);
	void removeAllNodes(IDManager& idManager);

	std::shared_ptr<Node>& findNodeById(ed::NodeId id);
	std::shared_ptr<Node>& findNodeByPinId(ed::PinId id);
	Pin& findPinById(ed::PinId id);
	std::shared_ptr<Node>& getMasterNode();
	std::list<std::shared_ptr<Node>> getHiddenNodes();

	void setNodePosition(std::shared_ptr<Node>& node, const ImVec2& pos);

	void render();

	template <class Archive>
	void serialize(Archive& archive);

	template <class Archive>
	void load(Archive& archive, IDManager& idManager);

private:
	void releaseNodeIds(IDManager& idManager, std::shared_ptr<Node>& node);
	void eraseNode(std::shared_ptr<Node>& node);
	void registerNodeIds(IDManager& idManager, std::shared_ptr<Node>& node);
};

// Template method definition

template<typename T>
std::shared_ptr<Node> NodeManager::addNode(IDManager& idManager) {
	std::shared_ptr<Node> node = std::make_shared<T>(&idManager);
	_nodes.push_back(node);
	return _nodes.back();
}

template <class Archive>
void NodeManager::serialize(Archive& archive) {
	for (std::shared_ptr<Node>& node : _nodes)
	{
		archive(
			cereal::make_nvp("node", node),
			cereal::make_nvp("node_position", ed::GetNodePosition(node->id))
		);
	}
}

template <class Archive>
void NodeManager::load(Archive& archive, IDManager& idManager) {
	try{
		while (true)
		{
			std::shared_ptr<Node> node;
			archive(node);
			ImVec2 pos;
			archive(pos);

			registerNodeIds(idManager, node);
			ed::SetNodePosition(node->id, pos);

			_nodes.push_back(node);
		}
	}
	catch (std::exception& e)
	{ }
}
