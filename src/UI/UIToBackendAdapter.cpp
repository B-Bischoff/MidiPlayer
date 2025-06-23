#include "UI/UIToBackendAdapter.hpp"

// Backend update methods

void UIToBackendAdapter::updateBackend(Master& master, NodeUIManagers& managers)
{
	//printTreesDiff(master, managers);
	Node& UIMaster = *(managers.node.getMasterNode());

	std::vector<BackendInstruction*> instructions;
	createInstructions(&master, &master, &UIMaster, managers, instructions);
	processInstructions(master, UIMaster, managers, instructions);
}

void UIToBackendAdapter::processInstructions(Master& master, Node& UIMaster, NodeUIManagers& managers, std::vector<BackendInstruction*>& instructions)
{
	for (const BackendInstruction* instruction : instructions)
	{
		if (dynamic_cast<const AddNode*>(instruction))
			addAudioComponent(master, UIMaster, managers, *dynamic_cast<const AddNode*>(instruction));
		else if (dynamic_cast<const RemoveNode*>(instruction))
			removeAudioComponent(master, managers.node, *dynamic_cast<const RemoveNode*>(instruction));
		else if (dynamic_cast<const UpdateNode*>(instruction))
			updateAudioComponent(master, managers.node, *dynamic_cast<const UpdateNode*>(instruction));
		else
		{
			Logger::log("UIToBackendAdapter", Error) << "Unrecognized backend instruction." << std::endl;
			exit(1);
		}

		delete instruction;
	}
}

// From component parameter, deletes component AND component childs that cannot be reached from master
void UIToBackendAdapter::removeUnreachableComponentAndInputs(AudioComponent* master, AudioComponent* branchRoot, AudioComponent* component, NodeManager& nodeManager)
{
	Components inputs = component->getInputs();

	auto it = inputs.begin();
	while (it != inputs.end())
	{
		if (master->idExists((*it)->id) == false) // Continue to delete branch as long as its connected to root
		{
			removeUnreachableComponentAndInputs(master, branchRoot, *it, nodeManager);
			inputs = component->getInputs();
			it = inputs.begin();
			if (it == inputs.end())
				break;
		}
		else
			it++;
	}

	// Update associated UI node to not point on this removed audio component.
	// UI node might not exists if it was deleted (obviously causing this audio component to be deleted)
	// Or it is not linked anymore but is still present in the node editor.
	std::shared_ptr<Node> node = nodeManager.findNodeByAudioComponentId(component->id);
	if (node)
		node->clearAudioComponentPointerAndId();

	master->removeComponentFromBranch(component, false);
	branchRoot->removeComponentFromBranch(component, true);
}

/*
 * This method goes through the audio backend tree (made of audio components) and compares it with the UI nodes tree.
 * Things can differ between trees: UI node can be deleted/added or have different properties than the its corresponding audio components.
 * In such cases, a backend instruction is created and added to the vector instructions passed in parameter.
*/
void UIToBackendAdapter::createInstructions(AudioComponent* master, AudioComponent* component, Node* node, NodeUIManagers& managers, std::vector<BackendInstruction*>& instructions, AudioComponent* parentNode)
{
	const int compareNodesReturn = compareNodes(component, node, parentNode->id);
	if (compareNodesReturn == 1)
	{
		RemoveNode* instruction = new RemoveNode(parentNode->id, component->id); assert(instruction);
		instructions.push_back(instruction);
		return; // Stop comparing trees branch when node differs
	}
	if (compareNodesReturn == 2)
	{
		UpdateNode* instruction = new UpdateNode(node->id); assert(instruction);
		instructions.push_back(instruction);
	}

	int currentInputIndex = 1;

	for (const ComponentInput& inputs : component->inputs) // loop over audio component inputs
	{
		std::unordered_set<int> visitedChild;

		for (AudioComponent* input : inputs) // loop over all the component plugged to one input
		{
			createInstructions(master, input, getNodeDirectChild(node, managers, input->id), managers, instructions, component);
			visitedChild.insert(input->id);
		}

		// Loop over node childs present in the UI tree but missing backend side
		if (node)
			processNodeLinks(master, node, managers, instructions, visitedChild, currentInputIndex);

		currentInputIndex++;
	}
}

void UIToBackendAdapter::processNodeLinks(AudioComponent* master, Node* node, NodeUIManagers& managers, std::vector<BackendInstruction*>& instructions, const std::unordered_set<int>& visitedChild, int currentInputIndex)
{
	const std::list<LinkInfo> nodeLinks = managers.link.findNodeLinks(managers.node, node->id, 1);
	for (const LinkInfo& link : nodeLinks)
	{
		const std::shared_ptr<Node>& inputNode = managers.node.findNodeByPinId(link.OutputId); assert(inputNode.get());
		const int linkInputIndex = node->getInputIndexFromPinId(link.InputId.Get()); assert(linkInputIndex != -1);

		if (currentInputIndex == linkInputIndex && visitedChild.find(inputNode->audioComponentId) == visitedChild.end())
			browseNodeBranch(master, *inputNode.get(), managers, instructions, node->id, currentInputIndex);
	}
}

/*
 * Compare audio component with its associated ui node.
 * Return:
 *     0 when ids and properties are similar
 *     1 when ids are different
 *     2 when properties are different
*/
int UIToBackendAdapter::compareNodes(AudioComponent* component, Node* node, const unsigned int& parentNodeId)
{
	bool ID_Match = false; // Used to indicate if UI node points to the correct audio component
	bool nodesPropertiesAreSimilar = false; // If id matches, compare the properties of the ui node and the matching audio component

	if (node != nullptr)
	{
		// Make an exception when comparing master. As this component doesn't have any parent no AddInstruction can be created.
		// This special case is handled in the addAudioComponent() method. Where master component and its node are manually linked together.
		ID_Match = (component->id == node->audioComponentId || component->id == 1);
		if (ID_Match)
			nodesPropertiesAreSimilar = (*node == component);
	}

	if (ID_Match == false)
		return 1;
	else if (nodesPropertiesAreSimilar == false && node->id != MASTER_NODE_ID) // Master node cannot be updated
		return 2;
	return 0;
}

void UIToBackendAdapter::browseNodeBranch(AudioComponent* master, const Node& node, NodeUIManagers& managers, std::vector<BackendInstruction*>& instructions, const unsigned int parentId, int inputIndex)
{
	Node* parentNode = managers.node.findNodeById(parentId).get();
	AudioComponent* parentAudioComponent = master->getAudioComponent(parentNode->audioComponentId);
	AudioComponent* childAudioComponent = master->getAudioComponent(node.audioComponentId);

	// If audioComponents are not parent/child (like in the UI), create a new ADD instruction.
	// parent or child audio components can be NULL (e.g when loading an instrument) in that case, create a new ADD instruction.
	if (!parentAudioComponent || !childAudioComponent || !parentAudioComponent->idIsDirectChild(childAudioComponent->id))
	{
		AddNode* instruction = new AddNode(node.id, parentId, inputIndex); assert(instruction);
		instructions.push_back(instruction);
	}

	const std::list<LinkInfo> nodeLinks = managers.link.findNodeLinks(managers.node, node.id, 1);

	for (const LinkInfo& link : nodeLinks) // Loop over links where node is an input
	{
		const std::shared_ptr<Node>& inputNode = managers.node.findNodeByPinId(link.OutputId); assert(inputNode.get());
		const int currentInputIndex = node.getInputIndexFromPinId(link.InputId.Get()); assert(currentInputIndex != -1);

		browseNodeBranch(master, *inputNode, managers, instructions, node.id, currentInputIndex);
	}
}

Node* UIToBackendAdapter::getNodeDirectChild(Node* node, NodeUIManagers& managers, const unsigned int id)
{
	if (!node) return nullptr;

	const std::list<LinkInfo> nodeLinks = managers.link.findNodeLinks(managers.node, node->id, 1);

	for (const LinkInfo& link : nodeLinks) // Loop over links where node is an input
	{
		const std::shared_ptr<Node>& inputNode = managers.node.findNodeByPinId(link.OutputId);
		assert(inputNode.get());

		if (inputNode->audioComponentId == id)
			return inputNode.get();
	}

	return nullptr;
}

// Process instruction methods

void UIToBackendAdapter::addAudioComponent(Master& master, Node& masterUI, NodeUIManagers& managers, const AddNode& instruction)
{
	//Logger::log("Add node instruction", Debug) << "id: " << instruction.UI_NODE_ID << " parent id: " << instruction.UI_PARENT_NODE_ID << " parent input id: " << instruction.UI_PARENT_NODE_INPUT_ID << std::endl;

	// Get UI Node from instruction data
	Node* node = managers.node.findNodeById(instruction.UI_NODE_ID).get();
	Node* parentNode = managers.node.findNodeById(instruction.UI_PARENT_NODE_ID).get();

	// If root UI node (id == 1) was just instanciated, set its audioComponentId to 1 which is always master id
	if (parentNode->id == 1 && parentNode->audioComponentId == 0)
		parentNode->audioComponentId = MASTER_NODE_ID;

	// Check if child AudioComponent already exists
	AudioComponent* newAudioComponent = master.getAudioComponent(node->audioComponentId);
	if (newAudioComponent == nullptr)
	{
		// Allocate new AudioComponent and link UI node to it
		const NodeInfo& nodeInfo = managers.node.getNodeInfo(node);
		newAudioComponent = nodeInfo.convertNodeToAudioComponent(node);
		node->audioComponent = newAudioComponent;
		node->audioComponentId = newAudioComponent->id;
	}

	// Link new AudioComponent with its parent
	AudioComponent* parentAudioComponent = master.getAudioComponent(parentNode->audioComponentId); assert(parentAudioComponent);
	if (!parentAudioComponent->idIsDirectChild(newAudioComponent->id))
		parentAudioComponent->addInput(instruction.UI_PARENT_NODE_INPUT_ID - 1, newAudioComponent);
	//else // [TODO] If in the future linking two nodes multiple time is desired: remove this check and add a post processing step to instructions list to remove duplication instructions.
	//	Logger::log("Add node instruction", Warning) << "Parent " << parentAudioComponent->id << " was already linked to child " << newAudioComponent->id << std::endl;
}

void UIToBackendAdapter::removeAudioComponent(Master& master, NodeManager& nodeManager, const RemoveNode& instruction)
{
	//Logger::log("Remove node", Error) << "parent: " << instruction.PARENT_COMPONENT_ID << " child: " << instruction.CHILD_COMPONENT_ID << std::endl;

	// Remove backend link between component and child
	AudioComponent* parentComponent = master.getAudioComponent(instruction.PARENT_COMPONENT_ID); assert(parentComponent);
	AudioComponent* childComponent = master.getAudioComponent(instruction.CHILD_COMPONENT_ID);
	if (!childComponent) // Child might already have been deleted by a previous instruction
	{
		//Logger::log("Backend Update", Warning) << "Child could not be found, skipping" << std::endl;
		return;
	}
	parentComponent->removeInput(childComponent);

	if (master.idExists(childComponent->id) == false)
	{
		// Removed child cannot be found from root so clean its branch
		removeUnreachableComponentAndInputs(&master, childComponent, childComponent, nodeManager);
	}
}

// Print tree methods

void UIToBackendAdapter::printTreesDiff(Master& master, NodeUIManagers& managers)
{
	Node& UIMaster = *(managers.node.getMasterNode());
	printTreesDiff(&master, &master, &UIMaster, managers);
}

void UIToBackendAdapter::printTreesDiff(AudioComponent* master, AudioComponent* component, Node* node, NodeUIManagers& managers, AudioComponent* parentNode, PrintTreeControls print)
{
	int comparisonReturn = compareNodes(component, node, parentNode->id);
	std::string color = comparisonReturn == 1 ? ANSI_FG_RED : comparisonReturn == 2 ? ANSI_FG_BLUE : ANSI_FG_WHITE;
	drawTree(print, component->componentName + " " + std::to_string(component->id), color);

	if (comparisonReturn == 1) // Stop comparing trees branch when node differs
		return;

	print.drawVertical.push_back(true); // Assume there are children by default

	// Get child count to know when to print branch end character '└'
	int childCount = getChildCount(component, node, managers);
	int processedChildren = 0;
	print.inputIndex = 1;

	for (const ComponentInput& inputs : component->inputs) // loop over audio component inputs
	{
		std::unordered_set<int> visitedChild;

		for (AudioComponent* input : inputs) // loop over all the component plugged to one input
		{
			upateProcessedChildrenCount(childCount, processedChildren, print.drawVertical);
			printTreesDiff(master, input, getNodeDirectChild(node, managers, input->id), managers, component, print.incrementDepth());
			visitedChild.insert(input->id);
		}

		if (node) // Loop over node childs present in the UI tree but missing backend side
		{
			// Get node direct childrens
			const std::list<LinkInfo> nodeLinks = managers.link.findNodeLinks(managers.node, node->id, 1);
			for (const LinkInfo& link : nodeLinks) // Loop over links where node is an input
			{
				const std::shared_ptr<Node>& inputNode = managers.node.findNodeByPinId(link.OutputId); assert(inputNode.get());
				const int linkInputIndex = node->getInputIndexFromPinId(link.InputId.Get()); assert(linkInputIndex != -1);

				if (print.inputIndex == linkInputIndex && visitedChild.find(inputNode->audioComponentId) == visitedChild.end())
				{
					upateProcessedChildrenCount(childCount, processedChildren, print.drawVertical);
					printNewNodes(master, *inputNode.get(), managers, node->id, print.incrementDepth());
				}
			}
		}

		print.inputIndex++;
	}
}

void UIToBackendAdapter::printNewNodes(AudioComponent* master, const Node& node, NodeUIManagers& managers, const unsigned int parentId, PrintTreeControls print)
{
	if (print.depth == 0) std::cout << "================== UI NODES TREE ==================" << std::endl << std::endl;

	const std::string GREEN = "\033[1;32m";

	Node* parentNode = managers.node.findNodeById(parentId).get();
	AudioComponent* parentAudioComponent = master->getAudioComponent(parentNode->audioComponentId);
	AudioComponent* childAudioComponent = master->getAudioComponent(node.audioComponentId);

	drawTree(print, node.name + " id: " + std::to_string(node.id) + " | comp id: " + std::to_string(node.audioComponentId), GREEN);

	print.drawVertical.push_back(true); // Assume there are children by default

	const std::list<LinkInfo> nodeLinks = managers.link.findNodeLinks(managers.node, node.id, 1);
	const int childCount = nodeLinks.size();

	int processedChildren = 0;
	for (const LinkInfo& link : nodeLinks) // Loop over links where node is an input
	{
		const std::shared_ptr<Node>& inputNode = managers.node.findNodeByPinId(link.OutputId); assert(inputNode.get());
		upateProcessedChildrenCount(childCount, processedChildren, print.drawVertical);

		print.inputIndex = node.getInputIndexFromPinId(link.InputId.Get());
		printNewNodes(master, *inputNode, managers, node.id, print.incrementDepth());
	}
}

void UIToBackendAdapter::updateAudioComponent(Master& master, NodeManager& nodeManager, const UpdateNode& instruction)
{
	//Logger::log("Update node", Info) << instruction.UI_ID << std::endl;
	const Node* node = nodeManager.findNodeById(instruction.UI_ID).get(); assert(node);
	AudioComponent* audioComponent = master.getAudioComponent(node->audioComponentId); assert(audioComponent);
	node->assignToAudioComponent(audioComponent);
}

void UIToBackendAdapter::drawTree(const PrintTreeControls& print, const std::string& text, const std::string& color)
{
	const std::string RESET = "\033[1;0m";

	for (int i = 0; i < print.depth; i++)
	{
		if (i == print.depth - 1 && print.drawVertical[i])
			std::cout << color << "├─" << print.inputIndex << "─";
		else if (i == print.depth - 1)
			std::cout << color << "└─" << print.inputIndex << "─";
		else if (print.drawVertical[i])
			std::cout << "│   ";
		else
			std::cout << "    ";
	}
	std::cout << text << RESET << std::endl;
}

unsigned int UIToBackendAdapter::getChildCount(AudioComponent* component, Node* node, NodeUIManagers& managers)
{
	int childCount = 0;

	if (component)
	{
		for (const auto& inputs : component->inputs)
			childCount += inputs.size();
	}
	if (node)
	{
		std::list<LinkInfo> nodeLinks = managers.link.findNodeLinks(managers.node, node->id, 1);
		if (nodeLinks.size() > childCount)
			childCount = nodeLinks.size();
	}
	return childCount;
}

void UIToBackendAdapter::upateProcessedChildrenCount(const int childCount, int& processedChildren, std::vector<bool>& drawVertical)
{
	processedChildren++;
	if (processedChildren == childCount)
		drawVertical.back() = false; // Stop drawing vertical for the last child
}
