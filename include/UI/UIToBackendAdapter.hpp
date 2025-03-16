#pragma once

#include <cstring>
#include <list>
#include <unordered_set>
#include <vector>

#include "AudioBackend/Components/Components.hpp"
#include "NodeManager.hpp"
#include "LinkManager.hpp"

struct NodeUIManagers {
	NodeManager& node;
	LinkManager& link;
};


struct BackendInstruction {
	virtual ~BackendInstruction() {}
};

struct AddNode : public BackendInstruction {
	unsigned int UI_NODE_ID;
	unsigned int UI_PARENT_NODE_ID;
	unsigned int UI_PARENT_NODE_INPUT_ID;
};

struct RemoveNode : public BackendInstruction {
	unsigned int PARENT_COMPONENT_ID;
	unsigned int CHILD_COMPONENT_ID;
};

struct UpdateNode : public BackendInstruction {
	unsigned int UI_ID;
	// NODE ID is also obviously required but this can be found using the UI node
};

class UIToBackendAdapter {
public:
	static void updateBackend(Master& master, NodeUIManagers& managers);

	static void printTreesDiff(Master& master, Node& node, LinkManager& linkManager, NodeManager& nodeManager);
	static void printTree(AudioComponent* master, const Node& node, NodeUIManagers& managers, std::vector<BackendInstruction*>& instructions, const unsigned int parentId, int depth = 0, int inputIndex = 1, std::vector<bool> drawVertical = {});
	static void deleteComponentAndInputs(AudioComponent* component, AudioComponent* master);
	static void removeUnreachableComponentAndInputs(AudioComponent* master, AudioComponent* branchRoot, AudioComponent* component);

private:
	static void addAudioComponent(Master& master, Node& masterUI, NodeUIManagers& managers, const AddNode& instruction);
	static void updateAudioComponent(Master& master, NodeManager& nodeManager, const UpdateNode& instruction);
	static void removeAudioComponent(Master& master, const RemoveNode& instruction);

	static void removeComponentFromBackend(AudioComponent* component, AudioComponent* componentToRemove, const bool deleteComponent = true, unsigned int depth = 0);
	static void updateBackendNode(AudioComponent& master, AudioComponent& component, Node& node, NodeManager& nodeManager, LinkManager& linkManager);
	static AudioComponent* allocateAudioComponent(Node& node);

	// Print tree helpers (defaults to white text color)
	static void drawTree(int depth, int inputIndex, std::vector<bool> drawVertical, const std::string& text, const std::string& color = "\033[1;37m");

	static void printUIDiff(AudioComponent* component, Node& node, LinkManager& linkManager, NodeManager& nodeManager, int depth = 0, int inputIndex = 1, std::vector<bool> drawVertical = {});
	static void printBackendDiff(AudioComponent* component, Node& node, LinkManager& linkManager, NodeManager& nodeManager, int depth = 0, int inputIndex = 1, std::vector<bool> drawVertical = {});

	static void createInstructions(AudioComponent* master, AudioComponent* component, Node* node, NodeUIManagers& managers, std::vector<BackendInstruction*>& instructions, AudioComponent* parentComponent = nullptr, int depth = 0, int inputIndex = 1, std::vector<bool> drawVertical = {});
	static void processInstructions(Master& master, Node& UIMaster, NodeUIManagers& managers, std::vector<BackendInstruction*>& instructions);

	static AudioComponent* getAudioComponent(AudioComponent* component, const unsigned int id);
	static bool idExists(AudioComponent* component, const unsigned int id);

	// Method actually find audio component id linked to node.
	// It does not check for the UI node internal id.
	static Node* getNode(Node& node, NodeManager& nodeManager, LinkManager& linkManager, const unsigned int audioComponentId);

	// Find a Node by its id by recursevly traversing the tree from root.
	// Return nullptr if it can't find it.
	static Node* getUINode(Node& root, NodeManager& nodeManager, LinkManager& linkManager, const unsigned int nodeId);

	static Node* getNodeDirectChild(Node* node, NodeUIManagers& managers, const unsigned int id);
	static bool idExists(Node& node, NodeManager& nodeManager, LinkManager& linkManager, const unsigned int id);
};
