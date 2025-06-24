#pragma once

#include <list>
#include <memory>
#include <typeindex>
#include <vector>
#include <unordered_map>

#include "nodes.hpp"
#include "IDManager.hpp"

#include "Logger.hpp"

struct NodeInfo {
	std::string name;
	std::function<Node*(IDManager*)> instantiateFunction;
	std::function<Node*(Node*)> instantiateCopyFunction;
	std::function<AudioComponent*(Node*)> convertNodeToAudioComponent;
};

// [TODO] give NodeManager and LinkManager their own IDManager ??
class NodeManager {
private:
	std::vector<std::shared_ptr<Node>> _nodes;

public:
	template <typename NodeType, typename AudioComponentType>
	void registerNode(const std::string& nodeName);

	// [TODO] This should not be public
	std::unordered_map<std::type_index, NodeInfo> _nodesInfo;

	template<typename T>
	std::shared_ptr<Node> addNode(IDManager& idManager);
	std::shared_ptr<Node> addNode(Node* node) {
		assert(node);
		std::shared_ptr<Node> nodeShared(node);
		_nodes.push_back(nodeShared);
		return _nodes.back();
	}
	void removeNode(IDManager& idManager, std::shared_ptr<Node>& node);
	void removeNode(IDManager& idManager, ed::NodeId id);
	void removeAllNodes(IDManager& idManager);

	std::shared_ptr<Node>& findNodeById(ed::NodeId id);
	std::shared_ptr<Node>& findNodeByPinId(ed::PinId id);
	std::shared_ptr<Node> findNodeByAudioComponentId(const unsigned int audioComponentId);
	Pin& findPinById(ed::PinId id);
	std::shared_ptr<Node>& getMasterNode();
	std::list<std::shared_ptr<Node>> getHiddenNodes();

	template<typename T>
	std::list<std::shared_ptr<Node>> getNodeOfType();

	NodeInfo getNodeInfo(Node* node) const;

	void setNodePosition(std::shared_ptr<Node>& node, const ImVec2& pos);

	void render(std::queue<Message>& messages);

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

template<typename NodeType, typename AudioComponentType>
void NodeManager::registerNode(const std::string& nodeName) {
	if (std::is_base_of<Node, NodeType>::value == false)
	{
		Logger::log("NodeManager", Error) << "registration failed, type NodeType (" << typeid(NodeType).name() << ") is not a child of Node struct" << std::endl;
		exit(1);
	}

	const std::type_index typeIndex = std::type_index(typeid(NodeType));

	NodeInfo info;

	if (_nodesInfo.find(typeIndex) != _nodesInfo.end())
	{
		Logger::log("NodeManager", Error) << "double registration of node type (" << typeid(NodeType).name() << ")" << std::endl;
		exit(1);
	}

	// Register instantion function of this new node type
	info.instantiateFunction = [](IDManager* idManager) -> Node* {
		return new NodeType(idManager);
	};

	// Register instantiation function based on an existing node
	info.instantiateCopyFunction = [](const Node* node) -> Node* {
		NodeType* newNode = new NodeType(*(dynamic_cast<const NodeType*>(node)));
		return newNode;
	};

	// Register function to convert UI Node to its associated AudioComponent
	info.convertNodeToAudioComponent = [](const Node* node) -> AudioComponentType* {
		AudioComponentType* audioComponent = new AudioComponentType;
		node->assignToAudioComponent(audioComponent);
		return audioComponent;
	};

	info.name = nodeName;

	_nodesInfo[typeIndex] = info;
}

template<typename T>
std::shared_ptr<Node> NodeManager::addNode(IDManager& idManager) {
	std::shared_ptr<Node> node = std::make_shared<T>(&idManager);
	_nodes.push_back(node);
	return _nodes.back();
}

template<typename T>
std::list<std::shared_ptr<Node>> NodeManager::getNodeOfType() {
	std::list<std::shared_ptr<Node>> list;

	for (std::shared_ptr<Node>& node : _nodes)
	{
		if (dynamic_cast<T*>(node.get()))
			list.push_back(node);
	}
	return list;
}

// Serialization methods

struct NodeData {
	std::shared_ptr<Node> node;
	ImVec2 position;

	template <class Archive>
	void serialize(Archive& archive) {
		archive(
			cereal::make_nvp("node", node),
			cereal::make_nvp("position", position)
		);
	}
};

template <class Archive>
void NodeManager::serialize(Archive& archive) {
	std::vector<NodeData> nodesData;
	for (std::shared_ptr<Node>& node : _nodes)
	{
		NodeData nodeData = { node, ed::GetNodePosition(node->id) };
		nodesData.push_back(nodeData);
	}
	archive(cereal::make_nvp("nodes", nodesData));
}

template <class Archive>
void NodeManager::load(Archive& archive, IDManager& idManager) {
	std::vector<NodeData> nodesData;
	archive(nodesData);

	for (NodeData& nodeData : nodesData)
	{
		registerNodeIds(idManager, nodeData.node);
		ed::SetNodePosition(nodeData.node->id, nodeData.position);

		_nodes.push_back(nodeData.node);
	}
}
