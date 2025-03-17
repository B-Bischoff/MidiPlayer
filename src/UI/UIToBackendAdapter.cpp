#include "UI/UIToBackendAdapter.hpp"

void UIToBackendAdapter::updateBackend(Master& master, NodeUIManagers& managers)
{
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
			removeAudioComponent(master, *dynamic_cast<const RemoveNode*>(instruction));
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
void UIToBackendAdapter::removeUnreachableComponentAndInputs(AudioComponent* master, AudioComponent* branchRoot, AudioComponent* component)
{
	Components inputs = component->getInputs();

	auto it = inputs.begin();
	while (it != inputs.end())
	{
		if (master->idExists((*it)->id) == false) // Continue to delete branch as long as its connected to root
		{
			Logger::log("removeUnreachableComponentAndInputs", Debug) << "Should delete " << (*it)->componentName << " " << (*it)->id << std::endl;
			removeUnreachableComponentAndInputs(master, branchRoot, *it);
			inputs = component->getInputs();
			it = inputs.begin();
			if (it == inputs.end())
				break;
		}
		else
		{
			Logger::log("deleteUnreachableComponentAndInputs", Debug) << "Preserving " << (*it)->id << std::endl;
			it++;
		}
	}
	master->removeComponentFromBranch(component, false);
	branchRoot->removeComponentFromBranch(component, true);
}

/*
 * This method goes through the audio backend tree (made of audio components) and compares it with the UI nodes tree.
 * Things can differ between trees: UI node can be deleted/added or have different properties than the its corresponding audio components.
 * In such cases, a backend instruction is created and added to the vector instructions passed in parameter.
*/
void UIToBackendAdapter::createInstructions(AudioComponent* master, AudioComponent* component, Node* node, NodeUIManagers& managers, std::vector<BackendInstruction*>& instructions, AudioComponent* parentNode, int depth, int inputIndex, std::vector<bool> drawVertical)
{
	const std::string WHITE = "\033[1;37m";
	const std::string RED = "\033[1;31m";
	const std::string BLUE = "\033[1;34m";
	const std::string GREEN = "\033[1;32m";
	const std::string RESET = "\033[1;0m";
	std::string color = WHITE;

	if (depth == 0) std::cout << "================== NEW DIFF METHOD ==================" << std::endl << std::endl;

	int comparisonReturn = compareNodes(component, node, parentNode->id, instructions);
	if (comparisonReturn == 1) color = RED; // Id mismatch
	if (comparisonReturn == 2) color = BLUE; // Properties mismatch

	drawTree(depth, inputIndex, drawVertical, component->componentName + " " + std::to_string(component->id), color);

	if (comparisonReturn == 1) // Stop comparing trees branch when node differs
		return;

	drawVertical.push_back(true); // Assume there are children by default

	// Get child count to know when to print branch end character '└'
	int childCount = 0;
	for (const auto& inputs : component->inputs)
		childCount += inputs.size();
	if (node)
	{
		std::list<LinkInfo> nodeLinks = managers.link.findNodeLinks(managers.node, node->id, 1);
		if (nodeLinks.size() > childCount)
			childCount = nodeLinks.size();
	}

	int currentInputIndex = 1;
	int processedChildren = 0;

	for (const ComponentInput& inputs : component->inputs) // loop over audio component inputs
	{
		std::unordered_set<int> visitedChild;

		for (AudioComponent* input : inputs) // loop over all the component plugged to one input
		{
			processedChildren++;
			if (processedChildren == childCount)
				drawVertical.back() = false; // Stop drawing vertical for the last child

			createInstructions(master, input, getNodeDirectChild(node, managers, input->id), managers, instructions, component, depth + 1, currentInputIndex, drawVertical);
			visitedChild.insert(input->id);
		}

		// Loop over node childs present in the UI tree but missing backend side
		if (node)
		{
			// Get node direct childrens
			const std::list<LinkInfo> nodeLinks = managers.link.findNodeLinks(managers.node, node->id, 1);
			for (const LinkInfo& link : nodeLinks) // Loop over links where node is an input
			{
				const std::shared_ptr<Node>& inputNode = managers.node.findNodeByPinId(link.OutputId);
				assert(inputNode.get());

				// Find input id
				int linkInputIndex = -1;
				for (int i = 0; i < node->inputs.size(); i++)
				{
					const Pin& pin = node->inputs[i];
					if (pin.id == link.InputId.Get())
					{
						linkInputIndex = i+1;
						break;
					}
				}
				assert(currentInputIndex != -1);

				if (currentInputIndex == linkInputIndex && visitedChild.find(inputNode->audioComponentId) == visitedChild.end())
				{
					processedChildren++;
					if (processedChildren == childCount)
						drawVertical.back() = false; // Stop drawing vertical for the last child

					printTree(master, *inputNode.get(), managers, instructions, node->id, depth + 1, currentInputIndex, drawVertical);
				}
			}
		}

		currentInputIndex++;
	}
}

/*
 * Compare audio component with its associated ui node.
 * Add an instruction to the instruction vector when difference occurs.
 * Return:
 *     0 when ids and properties are similar
 *     1 when ids are different
 *     2 when properties are different
*/
int UIToBackendAdapter::compareNodes(AudioComponent* component, Node* node, const unsigned int& parentNodeId, std::vector<BackendInstruction*>& instructions)
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
	{
		RemoveNode* instruction = new RemoveNode; assert(instruction);
		instruction->PARENT_COMPONENT_ID = parentNodeId;
		instruction->CHILD_COMPONENT_ID = component->id;
		instructions.push_back(instruction);
		return 1;
	}
	else if (nodesPropertiesAreSimilar == false && node->id != 1) // Master node cannot be updated
	{
		UpdateNode* instruction = new UpdateNode; assert(instruction);
		instruction->UI_ID = node->id;
		instructions.push_back(instruction);
		return 2;
	}
	return 0;
}

void UIToBackendAdapter::printTree(AudioComponent* master, const Node& node, NodeUIManagers& managers, std::vector<BackendInstruction*>& instructions, const unsigned int parentId, int depth, int inputIndex, std::vector<bool> drawVertical)
{
	if (depth == 0) std::cout << "================== UI NODES TREE ==================" << std::endl << std::endl;

	const std::string GREEN = "\033[1;32m";

	Node* parentNode = managers.node.findNodeById(parentId).get();
	AudioComponent* parentAudioComponent = master->getAudioComponent(parentNode->audioComponentId);
	AudioComponent* childAudioComponent = master->getAudioComponent(node.audioComponentId);

	// If audioComponents are not parent/child (like in the UI), create a new ADD instruction.
	// parent or child audio components can be NULL (e.g when loading an instrument) in that case, create a new ADD instruction.
	if (!parentAudioComponent || !childAudioComponent || !parentAudioComponent->idIsDirectChild(childAudioComponent->id))
	{
		AddNode* instruction = new AddNode; assert(instruction);
		instruction->UI_NODE_ID = node.id;
		instruction->UI_PARENT_NODE_ID = parentId;
		instruction->UI_PARENT_NODE_INPUT_ID = inputIndex;
		instructions.push_back(instruction);
	}

	drawTree(depth, inputIndex, drawVertical, node.name + " id: " + std::to_string(node.id) + " | comp id: " + std::to_string(node.audioComponentId), GREEN);

	drawVertical.push_back(true); // Assume there are children by default

	const std::list<LinkInfo> nodeLinks = managers.link.findNodeLinks(managers.node, node.id, 1);
	const int childCount = nodeLinks.size();

	int processedChildren = 0;
	for (const LinkInfo& link : nodeLinks) // Loop over links where node is an input
	{
		const std::shared_ptr<Node>& inputNode = managers.node.findNodeByPinId(link.OutputId);
		assert(inputNode.get());
		processedChildren++;
		if (processedChildren == childCount)
			drawVertical.back() = false; // Stop drawing vertical for the last child

		// Find input index
		int currentInputIndex = -1;
		for (int i = 0; i < node.inputs.size(); i++)
		{
			const Pin& pin = node.inputs[i];
			if (pin.id == link.InputId.Get())
			{
				currentInputIndex = i+1;
				break;
			}
		}
		assert(currentInputIndex != -1);

		printTree(master, *inputNode, managers, instructions, node.id, depth + 1, currentInputIndex, drawVertical);
	}
}

void UIToBackendAdapter::drawTree(int depth, int inputIndex, std::vector<bool> drawVertical, const std::string& text, const std::string& color)
{
	const std::string RESET = "\033[1;0m";

	for (int i = 0; i < depth; i++)
	{
		if (i == depth - 1 && drawVertical[i])
			std::cout << color << "├─" << inputIndex << "─";
		else if (i == depth - 1)
			std::cout << color << "└─" << inputIndex << "─";
		else if (drawVertical[i])
			std::cout << "│   ";
		else
			std::cout << "    ";
	}
	std::cout << text << RESET << std::endl;
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
	Logger::log("Add node instruction", Debug) << "id: " << instruction.UI_NODE_ID << " parent id: " << instruction.UI_PARENT_NODE_ID << " parent input id: " << instruction.UI_PARENT_NODE_INPUT_ID << std::endl;

	// Get UI Node from instruction data
	Node* node = managers.node.findNodeById(instruction.UI_NODE_ID).get();
	Node* parentNode = managers.node.findNodeById(instruction.UI_PARENT_NODE_ID).get();

	// If root UI node (id == 1) was just instanciated, set its audioComponentId to 1 which is always master id
	if (parentNode->id == 1 && parentNode->audioComponentId == 0)
		parentNode->audioComponentId = 1;

	// Check if child AudioComponent already exists
	AudioComponent* newAudioComponent = master.getAudioComponent(node->audioComponentId);
	if (newAudioComponent == nullptr)
	{
		// Allocate new AudioComponent and link UI node to it
		newAudioComponent = node->convertNodeToAudioComponent(); assert(newAudioComponent);
		node->audioComponentId = newAudioComponent->id;
	}

	// Link new AudioComponent with its parent
	AudioComponent* parentAudioComponent = master.getAudioComponent(parentNode->audioComponentId); assert(parentAudioComponent);
	if (!parentAudioComponent->idIsDirectChild(newAudioComponent->id))
		parentAudioComponent->addInput(instruction.UI_PARENT_NODE_INPUT_ID - 1, newAudioComponent);
	else // [TODO] If in the future linking two nodes multiple time is desired: remove this check and add a post processing step to instructions list to remove duplication instructions.
		Logger::log("Add node instruction", Warning) << "Parent " << parentAudioComponent->id << " was already linked to child " << newAudioComponent->id << std::endl;
}

void UIToBackendAdapter::removeAudioComponent(Master& master, const RemoveNode& instruction)
{
	Logger::log("Remove node", Error) << "parent: " << instruction.PARENT_COMPONENT_ID << " child: " << instruction.CHILD_COMPONENT_ID << std::endl;

	// Remove backend link between component and child
	AudioComponent* parentComponent = master.getAudioComponent(instruction.PARENT_COMPONENT_ID); assert(parentComponent);
	AudioComponent* childComponent = master.getAudioComponent(instruction.CHILD_COMPONENT_ID);
	if (!childComponent) // Child might already have been deleted by a previous instruction
	{
		Logger::log("Backend Update", Warning) << "Child could not be found, skipping" << std::endl;
		return;
	}
	parentComponent->removeInput(childComponent);

	if (master.idExists(childComponent->id) == false)
	{
		// Removed child cannot be found from root so clean its branch
		removeUnreachableComponentAndInputs(&master, childComponent, childComponent);
	}
}

void UIToBackendAdapter::updateAudioComponent(Master& master, NodeManager& nodeManager, const UpdateNode& instruction)
{
	Logger::log("Update node", Info) << instruction.UI_ID << std::endl;
	const Node* node = nodeManager.findNodeById(instruction.UI_ID).get(); assert(node);
	AudioComponent* audioComponent = master.getAudioComponent(node->audioComponentId); assert(audioComponent);
	node->assignToAudioComponent(audioComponent);
}
