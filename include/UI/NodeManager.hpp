#pragma once

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
		std::shared_ptr<Node> addNode(IDManager& idManager)
{
	std::shared_ptr<Node> node = std::make_shared<T>(&idManager);
	_nodes.push_back(node);

	return _nodes.back();
}
	void removeNode(IDManager& idManager, std::shared_ptr<Node>& node);
	void removeNode(IDManager& idManager, ed::NodeId id);
	void removeAllNodes(IDManager& idManager);

	std::shared_ptr<Node>& findNodeById(ed::NodeId id);
	std::shared_ptr<Node>& findNodeByPinId(ed::PinId id);
	Pin& findPinById(ed::PinId id);
	std::shared_ptr<Node>& getMasterNode();

	void setNodePosition(std::shared_ptr<Node>& node, const ImVec2& pos);

	void render();

private:
	void releaseNodeIds(IDManager& idManager, std::shared_ptr<Node>& node);
	void eraseNode(std::shared_ptr<Node>& node);
};
