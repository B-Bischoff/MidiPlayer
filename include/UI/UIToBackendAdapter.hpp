#pragma once

#include <cstring>
#include <list>
#include <unordered_set>
#include <vector>

#include "colors.hpp"
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

	AddNode(unsigned int UI_NODE_ID = 0, unsigned int UI_PARENT_NODE_ID = 0, unsigned int UI_PARENT_NODE_INPUT_ID = 0)
		: UI_NODE_ID(UI_NODE_ID), UI_PARENT_NODE_ID(UI_PARENT_NODE_ID), UI_PARENT_NODE_INPUT_ID(UI_PARENT_NODE_INPUT_ID) { }
};

struct RemoveNode : public BackendInstruction {
	unsigned int PARENT_COMPONENT_ID;
	unsigned int CHILD_COMPONENT_ID;

	RemoveNode(unsigned int PARENT_COMPONENT_ID = 0, unsigned int CHILD_COMPONENT_ID = 0)
		: PARENT_COMPONENT_ID(PARENT_COMPONENT_ID), CHILD_COMPONENT_ID(CHILD_COMPONENT_ID) { }
};

struct UpdateNode : public BackendInstruction {
	unsigned int UI_ID;
	// NODE ID is also obviously required but this can be found using the UI node

	UpdateNode(unsigned int UI_ID = 0)
		: UI_ID(UI_ID) { }
};

struct PrintTreeControls {
	int depth = 0;
	int inputIndex = 1;
	std::vector<bool> drawVertical = {};

	PrintTreeControls incrementDepth()
	{
		PrintTreeControls newPrint = *this;
		newPrint.depth++;
		return newPrint;
	}
};

class UIToBackendAdapter {
public:
	static void updateBackend(Master& master, NodeUIManagers& managers);

	static void printTreesDiff(Master& master, NodeUIManagers& managers);
	static void deleteComponentAndInputs(AudioComponent* component, AudioComponent* master);

private:
	static void createInstructions(AudioComponent* master, AudioComponent* component, Node* node, NodeUIManagers& managers, std::vector<BackendInstruction*>& instructions, AudioComponent* parentComponent = nullptr);
	static void processNodeLinks(AudioComponent* master, Node* node, NodeUIManagers& managers, std::vector<BackendInstruction*>& instructions, const std::unordered_set<int>& visitedChild, int currentInputIndex);
	static void browseNodeBranch(AudioComponent* master, const Node& node, NodeUIManagers& managers, std::vector<BackendInstruction*>& instructions, const unsigned int parentId, int inputIndex = 1);
	static int compareNodes(AudioComponent* component, Node* node, const unsigned int& parentNodeId);

	static void processInstructions(Master& master, Node& UIMaster, NodeUIManagers& managers, std::vector<BackendInstruction*>& instructions);
	static void addAudioComponent(Master& master, Node& masterUI, NodeUIManagers& managers, const AddNode& instruction);
	static void updateAudioComponent(Master& master, NodeManager& nodeManager, const UpdateNode& instruction);
	static void removeAudioComponent(Master& master, const RemoveNode& instruction);

	static Node* getNodeDirectChild(Node* node, NodeUIManagers& managers, const unsigned int id);
	static void removeUnreachableComponentAndInputs(AudioComponent* master, AudioComponent* branchRoot, AudioComponent* component);

	// Print tree helpers
	static void printTreesDiff(AudioComponent* master, AudioComponent* component, Node* node, NodeUIManagers& managers, AudioComponent* parentNode = nullptr, PrintTreeControls print = {});
	static void printNewNodes(AudioComponent* master, const Node& node, NodeUIManagers& managers, const unsigned int parentId, PrintTreeControls print = {});
	static void drawTree(const PrintTreeControls& print, const std::string& text, const std::string& color = ANSI_FG_WHITE);
	static void printNodeLinks(AudioComponent* master, Node* node, NodeUIManagers& managers, std::vector<BackendInstruction*>& instructions, const std::unordered_set<int>& visitedChild, int currentInputIndex);
	static unsigned int getChildCount(AudioComponent* component, Node* node, NodeUIManagers& managers);
	static void upateProcessedChildrenCount(const int childCount, int& processedChildren, std::vector<bool>& drawVertical);
};
