#include "UI/NodeManager.hpp"

void NodeManager::releaseNodeIds(IDManager& idManager, std::shared_ptr<Node>& node)
{
	assert(node && "[NodeManager] releaseNodeIds must not receive empty pointer");

	// Release node itself
	idManager.releaseID(node->id);

	// Release node pins
	for (Pin& pin : node->inputs)
		idManager.releaseID(pin.id);
	for (Pin& pin : node->outputs)
		idManager.releaseID(pin.id);
}

/*
 * Remove specified node from the nodes vector.
 * Make sure to release node ids before calling this method (see releaseNodeIds).
*/
void NodeManager::eraseNode(std::shared_ptr<Node>& node)
{
	assert(node && "[NodeManager] eraseNode must not receive empty pointer");

	std::vector<std::shared_ptr<Node>>::iterator it = std::find(_nodes.begin(), _nodes.end(), node);
	assert(it != _nodes.end() && "[NodeManager] removeNode must not be called on non-existing node");

	_nodes.erase(it);
}

/*
 * delete node from the nodes vector after releasing every ids registered by that node.
*/
void NodeManager::removeNode(IDManager& idManager, std::shared_ptr<Node>& node)
{
	releaseNodeIds(idManager, node);
	eraseNode(node);
}

void NodeManager::removeNode(IDManager& idManager, ed::NodeId id)
{
	removeNode(idManager, findNodeById(id));
}

void NodeManager::removeAllNodes(IDManager& idManager)
{
	while (_nodes.size())
		removeNode(idManager, _nodes.back());
}

std::shared_ptr<Node>& NodeManager::findNodeById(ed::NodeId id)
{
	for (std::shared_ptr<Node>& node : _nodes)
	{
		if (node->id == id.Get())
			return node;
	}
	assert(0 && "[NodeManager]: findNodeById must not but be called on non-existing node-id.");
	return _nodes[0];
}

std::shared_ptr<Node>& NodeManager::findNodeByPinId(ed::PinId id)
{
	for (std::shared_ptr<Node>& node : _nodes)
	{
		for (Pin& pin : node->inputs)
			if (pin.id == id.Get())
				return node;
		for (Pin& pin : node->outputs)
			if (pin.id == id.Get())
				return node;
	}
	assert(0 && "[NodeManager]: findNodeByPinId must must not be called on non-existing pin-id.");
	return _nodes[0];
}

Pin& NodeManager::findPinById(ed::PinId id)
{
	for (std::shared_ptr<Node>& node : _nodes)
	{
		for (Pin& pin : node->inputs)
			if (pin.id == id.Get())
				return pin;
		for (Pin& pin : node->outputs)
			if (pin.id == id.Get())
				return pin;
	}
	assert(0 && "[NodeManager]: findPinById must must not be called on non-existing pin-id.");
	return _nodes[0]->inputs[0];
}

std::shared_ptr<Node>& NodeManager::getMasterNode()
{
	for (std::shared_ptr<Node>& node : _nodes)
	{
		if (node->type == UI_NodeType::MasterUI)
			return node;
	}
	assert(0 && "[NodeManager] must have a master node");
	return _nodes[0];
}

std::list<std::shared_ptr<Node>> NodeManager::getHiddenNodes()
{
	std::list<std::shared_ptr<Node>> list;

	for (auto& node : _nodes)
	{
		if (node->hidden)
			list.push_back(node);
	}

	return list;
}

void NodeManager::setNodePosition(std::shared_ptr<Node>& node, const ImVec2& pos)
{
	assert(node && "[NodeManager] setNodePosition must not receive empty pointer");
	ed::SetNodePosition(node->id, pos);
}

void NodeManager::setAudioPlayerNodeFileId(const id fileId)
{
	for (std::shared_ptr<Node>& node : _nodes)
	{
		if (node->type == FilePlayerUI)
		{
			std::shared_ptr<FilePlayerNode> filePlayerNode =  std::static_pointer_cast<FilePlayerNode>(node);
			if (std::static_pointer_cast<FilePlayerNode>(node)->wantsToLoadFile)
			{
				filePlayerNode->fileId = fileId;
				filePlayerNode->wantsToLoadFile = false;
				Node::propertyChanged = true;
			}
		}
	}
}

void NodeManager::resetAudioPlayerNodeFileLoad()
{
	for (std::shared_ptr<Node>& node : _nodes)
	{
		if (node->type == FilePlayerUI)
			std::static_pointer_cast<FilePlayerNode>(node)->wantsToLoadFile = false;
	}
}

void NodeManager::render(std::queue<Message>& messages)
{
	for (std::shared_ptr<Node>& node : _nodes)
	{
		if (!node->hidden)
			node->render(messages);
	}
}

void NodeManager::registerNodeIds(IDManager& idManager, std::shared_ptr<Node>& node)
{
	assert(node);

	// Register node itself
	node->id = idManager.getID(node->id);
	assert(node->id != INVALID_ID && "Could not load node");

	// Register node pins
	for (Pin& pin : node->inputs)
	{
		pin.id = idManager.getID(pin.id);
		assert(pin.id != INVALID_ID && "Could not load node");
	}
	for (Pin& pin : node->outputs)
	{
		pin.id = idManager.getID(pin.id);
		assert(pin.id != INVALID_ID && "Could not load node");
	}
}

NodeInfo NodeManager::getNodeInfo(Node* node) const
{
	auto& dereferencedNode = *node;
	const std::type_index typeIndex = std::type_index(typeid(dereferencedNode));
	auto it = _nodesInfo.find(typeIndex);
	if (it == _nodesInfo.end())
	{
		Logger::log("NodeManager", Error) << "Could not find node type " << typeid(dereferencedNode).name() << std::endl;
		exit(1);
	}
	return it->second;
}
