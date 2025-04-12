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
};

// [TODO] give NodeManager and LinkManager their own IDManager ??
class NodeManager {
private:
	std::vector<std::shared_ptr<Node>> _nodes;

public:
	template <typename T>
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
	Pin& findPinById(ed::PinId id);
	std::shared_ptr<Node>& getMasterNode();
	std::list<std::shared_ptr<Node>> getHiddenNodes();

	NodeInfo getNodeInfo(Node* node) const;

	void setNodePosition(std::shared_ptr<Node>& node, const ImVec2& pos);

	void setAudioPlayerNodeFileId(const id fileId);
	void resetAudioPlayerNodeFileLoad();

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

template<typename T>
void NodeManager::registerNode(const std::string& nodeName) {
	if (std::is_base_of<Node, T>::value == false)
	{
		Logger::log("NodeManager", Error) << "registration failed, type T (" << typeid(T).name() << ") is not a child of Node struct" << std::endl;
		exit(1);
	}

	const std::type_index typeIndex = std::type_index(typeid(T));

	NodeInfo info;

	if (_nodesInfo.find(typeIndex) != _nodesInfo.end())
	{
		Logger::log("NodeManager", Error) << "double registration of type T (" << typeid(T).name() << ")" << std::endl;
		exit(1);
	}

	// Register instantion function of this new node type
	info.instantiateFunction = [](IDManager* idManager) -> Node* {
		return new T(idManager);
	};

	// Register instantion function based on an existing node
	info.instantiateCopyFunction = [](const Node* node) -> Node* {
		T* newNode = new T;
		*newNode = *(dynamic_cast<const T*>(node));
		return newNode;
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

// Serialization methods

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
