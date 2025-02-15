#include "UI/UIToBackendAdapter.hpp"

void UIToBackendAdapter::updateBackend(Master& master, NodeManager& nodeManager, LinkManager& linkManager)
{
	Node& UIMaster = *nodeManager.getMasterNode();

	printTree(&master);
	//printTree(UIMaster, linkManager, nodeManager);
	printTreesDiff(&master, UIMaster, linkManager, nodeManager);


	Logger::log("Manual cleanup", Warning) << std::endl;
	Components inputs = master.getInputs();
	auto it = inputs.begin();
	while (it != inputs.end())
	{
		Logger::log("AA") << (*it)->componentName << " " << (*it)->id << std::endl;
		if (idExists(&master, (*it)->id))
			deleteComponentAndInputs(*it, &master);
		else
			it++;
		inputs = master.getInputs();
		it = inputs.begin();
		if (it == inputs.end())
			break;
	}
	Logger::log("Manual cleanup done", Warning) << std::endl;

	updateBackendNode(master, master, UIMaster, nodeManager, linkManager);

	//printTree(&master);
}

// [TODO] This belongs to instruments class
void UIToBackendAdapter::removeComponentFromBackend(AudioComponent* component, AudioComponent* componentToRemove, const bool deleteComponent, unsigned int depth)
{
	Components inputs = component->getInputs();
	for (AudioComponent* input : inputs)
		removeComponentFromBackend(input, componentToRemove, deleteComponent, depth + 1);

	component->removeInput(componentToRemove);

	if (deleteComponent && depth == 0)
	{
		Logger::log("Remove Component from Backend") << "REMOVING " << componentToRemove->id << std::endl;
		delete componentToRemove;
	}
}

// From component parameter, deletes component AND component childs that cannot be reached from master parameter
void UIToBackendAdapter::deleteUnreachableComponentAndInputs(AudioComponent* master, AudioComponent* branchRoot, AudioComponent* component)
{
	Components inputs = component->getInputs();

	auto it = inputs.begin();
	while (it != inputs.end())
	{
		if (idExists(master, (*it)->id) == false) // Continue to delete branch as long as its connected to root
		{
			Logger::log("deleteUnreachableComponentAndInputs", Debug) << "Should delete " << (*it)->componentName << " " << (*it)->id << std::endl;
			deleteUnreachableComponentAndInputs(master, branchRoot, *it);
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
	removeComponentFromBackend(master, component, false);
	removeComponentFromBackend(branchRoot, component);
}

// [TODO] This belongs to instruments class
void UIToBackendAdapter::deleteComponentAndInputs(AudioComponent* component, AudioComponent* master)
{
	Components inputs = component->getInputs();

	auto it = inputs.begin();
	while (it != inputs.end())
	{
		if (idExists(master, (*it)->id))
			deleteComponentAndInputs(*it, master);
		else
			it++;
		inputs = component->getInputs();
		it = inputs.begin();
		if (it == inputs.end())
			break;
	}

	removeComponentFromBackend(master, component);
}

void UIToBackendAdapter::updateBackendNode(AudioComponent& master, AudioComponent& component, Node& node, NodeManager& nodeManager, LinkManager& linkManager)
{
	// Get all links where node is the input
	std::list<LinkInfo> nodeLinks = linkManager.findNodeLinks(nodeManager, node.id, 1);

	// Tells UI node which audioComponent it points to
	node.audioComponentId = component.id;

	for (LinkInfo& link : nodeLinks)
	{
		std::shared_ptr<Node>& inputNode = nodeManager.findNodeByPinId(link.OutputId);
		assert(inputNode);
		AudioComponent* newInput = getAudioComponent(&master, inputNode->audioComponentId);
		const bool inputAlreadyExists = newInput != nullptr;
		if (newInput == nullptr)
			newInput = allocateAudioComponent(*inputNode);

		// Find input name
		int inputId = -1;
		for (Pin& pin : node.inputs)
		{
			if (pin.id == link.InputId.Get())
				inputId = pin.inputId;
		}

		if (inputId < 0)
		{
			Logger::log("UIToBackendAdapter", Error) << "InputId should not be undefined" << std::endl;
			exit(1);
		}

		component.addInput(inputId, newInput);

		/*
		* The inputAlreadyExists variable avoid to update node and add children multiple times.
		* Without it, the following pattern would update two times components B & D.
		* Component A (root) -------------------> Component B -> Component D
		*                   `---> Component C -->'
		*/
		if (!inputAlreadyExists)
			updateBackendNode(master, *newInput, *inputNode, nodeManager, linkManager);
	}
}

AudioComponent* UIToBackendAdapter::allocateAudioComponent(Node& node)
{
	AudioComponent* audioComponent = node.convertNodeToAudioComponent();
	if (audioComponent == nullptr)
	{
		Logger::log("UIToBackendAdapter", Error) << "Failed audio component allocation" << std::endl;
		exit(1);
	}

	return audioComponent;
}

void UIToBackendAdapter::printTreesDiff(AudioComponent* component, Node& node, LinkManager& linkManager, NodeManager& nodeManager)
{
	std::vector<BackendInstruction*> instructions;
	testing(component, &node, linkManager, nodeManager, instructions);

	for (const BackendInstruction* instruction : instructions)
	{
		if (dynamic_cast<const AddNode*>(instruction))
		{
			const AddNode* addNode = dynamic_cast<const AddNode*>(instruction);
			Logger::log("Add node", Debug) << "id: " << addNode->UI_NODE_ID << " parent id: " << addNode->UI_PARENT_NODE_ID << " parent input id: " << addNode->UI_PARENT_NODE_INPUT_ID << std::endl;
			addAudioComponent(component, node, linkManager, nodeManager, *addNode);
		}
		else if (dynamic_cast<const RemoveNode*>(instruction))
		{
			const RemoveNode* removeNode = dynamic_cast<const RemoveNode*>(instruction);
			Logger::log("Remove node", Error) << "parent: " << removeNode->PARENT_COMPONENT_ID << " child: " << removeNode->CHILD_COMPONENT_ID << std::endl;

			//if (idExists(component, removeNode->COMPONENT_ID))
			//	deleteComponentAndInputs(getAudioComponent(component, removeNode->COMPONENT_ID), component);


			// Remove backend link between component and child
			AudioComponent* parentComponent = getAudioComponent(component, removeNode->PARENT_COMPONENT_ID); assert(parentComponent);
			AudioComponent* childComponent = getAudioComponent(component, removeNode->CHILD_COMPONENT_ID);
			if (!childComponent)
			{
				delete instruction;
				Logger::log("Backend Update", Warning) << "Child could not be found, skipping" << std::endl;
				continue;
			}
			parentComponent->removeInput(childComponent);

			if (idExists(component, childComponent->id) == false)
			{
				// Removed child cannot be found from root => clean its branch
				Logger::log("REMOVE") << "Child not found from root" << std::endl;
				//deleteComponentAndInputs(childComponent, childComponent);
				deleteUnreachableComponentAndInputs(component, childComponent, childComponent);
				Logger::log("REMOVE") << "Deleted branch" << std::endl;
			}
			else
				Logger::log("REMOVE") << "Child found" << std::endl;
		}
		else if (dynamic_cast<const UpdateNode*>(instruction))
		{
			const UpdateNode* updateNode = dynamic_cast<const UpdateNode*>(instruction);
			Logger::log("Update node", Info) << updateNode->UI_ID << std::endl;
		}
		else
			assert(0);

		delete instruction;
	}
}

void UIToBackendAdapter::testing(AudioComponent* component, Node* node, LinkManager& linkManager, NodeManager& nodeManager, std::vector<BackendInstruction*>& instructions, AudioComponent* parentNode, int depth, int inputIndex, std::vector<bool> drawVertical)
{
	if (depth == 0) std::cout << "================== NEW DIFF METHOD ==================" << std::endl << std::endl;

	bool ID_Match = false;
	bool nodesValueDiffers = true;

	if (node != nullptr)
	{
		ID_Match = (component->id == node->audioComponentId || component->id == 1); // Make exception for Master
		if (ID_Match)
			nodesValueDiffers = !(*node == component);
	}

	const std::string WHITE = "\033[1;37m";
	const std::string RED = "\033[1;31m";
	const std::string BLUE = "\033[1;34m";
	const std::string GREEN = "\033[1;32m";
	const std::string RESET = "\033[1;0m";
	std::string color = WHITE;

	if (!ID_Match)
	{
		RemoveNode* instruction = new RemoveNode; assert(instruction);
		assert(parentNode);
		instruction->PARENT_COMPONENT_ID = parentNode->id;
		instruction->CHILD_COMPONENT_ID = component->id;
		instructions.push_back(instruction);

		color = RED;
	}
	else if (nodesValueDiffers)
	{
		UpdateNode* instruction = new UpdateNode; assert(instruction);
		instruction->UI_ID = node->id;
		instructions.push_back(instruction);

		color = BLUE;
	}
	drawTree(depth, inputIndex, drawVertical, component->componentName + " " + std::to_string(component->id), color);

	if (!ID_Match)
		return;

	drawVertical.push_back(true); // Assume there are children by default

	int childCount = 0;
	for (const auto& inputs : component->inputs)
		childCount += inputs.size();
	if (node)
	{
		std::list<LinkInfo> nodeLinks = linkManager.findNodeLinks(nodeManager, node->id, 1);
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

			testing(input, getNodeDirectChild(node, nodeManager, linkManager, input->id), linkManager, nodeManager, instructions, component, depth + 1, currentInputIndex, drawVertical);
			visitedChild.insert(input->id);
		}

		// Loop over node childs present in the UI tree but missing backend side
		if (node)
		{
			// Get node direct childrens
			const std::list<LinkInfo> nodeLinks = linkManager.findNodeLinks(nodeManager, node->id, 1);
			for (const LinkInfo& link : nodeLinks) // Loop over links where node is an input
			{
				const std::shared_ptr<Node>& inputNode = nodeManager.findNodeByPinId(link.OutputId);
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

					//drawTree(depth + 1, inputIndex, drawVertical, inputNode->name, GREEN);
					printTree(*inputNode.get(), linkManager, nodeManager, instructions, node->id, depth + 1, currentInputIndex, drawVertical);
					//testing(nullptr, inputNode.get(), linkManager, nodeManager, depth + 1, currentInputIndex, drawVertical);
				}
			}
		}

		currentInputIndex++;
	}
}

void UIToBackendAdapter::printUIDiff(AudioComponent* component, Node& node, LinkManager& linkManager, NodeManager& nodeManager, int depth, int inputIndex, std::vector<bool> drawVertical)
{
	if (depth == 0) std::cout << "================== UI NODES TREE ==================" << std::endl << std::endl;

	const std::string RED = "\033[1;31m";
	const std::string RESET = "\033[1;0m";
	const std::string GREEN = "\033[1;32m";
	if (!idExists(component, node.audioComponentId))
		std::cout << GREEN;
	drawTree(depth, inputIndex, drawVertical, node.name + " id: " + std::to_string(node.id) + " | comp id: " + std::to_string(node.audioComponentId));
	std::cout << RESET << std::flush;

	drawVertical.push_back(true); // Assume there are children by default

	std::list<LinkInfo> nodeLinks = linkManager.findNodeLinks(nodeManager, node.id, 1);
	const int childCount = nodeLinks.size();

	int processedChildren = 0;
	for (const LinkInfo& link : nodeLinks) // Loop over links where node is an input
	{
		std::shared_ptr<Node>& inputNode = nodeManager.findNodeByPinId(link.OutputId);
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

		printUIDiff(component, *inputNode, linkManager, nodeManager, depth + 1, currentInputIndex, drawVertical);
	}
}

void UIToBackendAdapter::printBackendDiff(AudioComponent* component, Node& node, LinkManager& linkManager, NodeManager& nodeManager, int depth, int inputIndex, std::vector<bool> drawVertical)
{
	if (depth == 0) std::cout << "================== BACKEND TREE ==================" << std::endl << std::endl;

	const std::string RED = "\033[1;31m";
	const std::string RESET = "\033[1;0m";
	const std::string GREEN = "\033[1;32m";
	//if (!component->idIsDirectChild(node.audioComponentId))
	if (!idExists(node, nodeManager, linkManager, component->id))
		std::cout << RED;
	drawTree(depth, inputIndex, drawVertical, component->componentName + " " + std::to_string(component->id));
	std::cout << RESET << std::flush;

	drawVertical.push_back(true); // Assume there are children by default
	int childCount = 0;
	for (const auto& inputs : component->inputs)
		childCount += inputs.size();

	int currentInputIndex = 1;
	int processedChildren = 0;
	for (const ComponentInput& inputs : component->inputs) // loop over component inputs
	{
		for (AudioComponent* input : inputs) // loop over all the component plugged to one input
		{
			processedChildren++;
			if (processedChildren == childCount)
				drawVertical.back() = false; // Stop drawing vertical for the last child

			printBackendDiff(input, node, linkManager, nodeManager, depth + 1, currentInputIndex, drawVertical);
		}
		currentInputIndex++;
	}
}

void UIToBackendAdapter::printTree(const AudioComponent* component, int depth, int inputIndex, std::vector<bool> drawVertical)
{
	if (depth == 0) std::cout << "================== BACKEND TREE ==================" << std::endl << std::endl;

	//drawTree(depth, inputIndex, drawVertical, component->componentName + " " + std::to_string(component->id));
	drawTree(depth, inputIndex, drawVertical, component->componentName + " " + std::to_string(component->id));

	drawVertical.push_back(true); // Assume there are children by default
	int childCount = 0;
	for (const auto& inputs : component->inputs)
		childCount += inputs.size();

	int currentInputIndex = 1;
	int processedChildren = 0;
	for (const ComponentInput& inputs : component->inputs) // loop over component inputs
	{
		for (const AudioComponent* input : inputs) // loop over all the component plugged to one input
		{
			processedChildren++;
			if (processedChildren == childCount)
				drawVertical.back() = false; // Stop drawing vertical for the last child

			printTree(input, depth + 1, currentInputIndex, drawVertical);
		}
		currentInputIndex++;
	}
}

void UIToBackendAdapter::printTree(const Node& node, LinkManager& linkManager, NodeManager& nodeManager, std::vector<BackendInstruction*>& instructions, const unsigned int parentId, int depth, int inputIndex, std::vector<bool> drawVertical)
{
	if (depth == 0) std::cout << "================== UI NODES TREE ==================" << std::endl << std::endl;

	const std::string GREEN = "\033[1;32m";

	AddNode* instruction = new AddNode; assert(instruction);
	instruction->UI_NODE_ID = node.id;
	instruction->UI_PARENT_NODE_ID = parentId;
	instruction->UI_PARENT_NODE_INPUT_ID = inputIndex;
	instructions.push_back(instruction);

	drawTree(depth, inputIndex, drawVertical, node.name + " id: " + std::to_string(node.id) + " | comp id: " + std::to_string(node.audioComponentId), GREEN);

	drawVertical.push_back(true); // Assume there are children by default

	const std::list<LinkInfo> nodeLinks = linkManager.findNodeLinks(nodeManager, node.id, 1);
	const int childCount = nodeLinks.size();

	int processedChildren = 0;
	for (const LinkInfo& link : nodeLinks) // Loop over links where node is an input
	{
		const std::shared_ptr<Node>& inputNode = nodeManager.findNodeByPinId(link.OutputId);
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

		printTree(*inputNode, linkManager, nodeManager, instructions, node.id, depth + 1, currentInputIndex, drawVertical);
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

AudioComponent* UIToBackendAdapter::getAudioComponent(AudioComponent* component, const unsigned int id)
{
	if (component->id == id)
		return component;

	for (const ComponentInput& inputs : component->inputs) // loop over component inputs
	{
		for (AudioComponent* input : inputs) // loop over all the component plugged to one input
		{
			AudioComponent* foundAudioComponent =  getAudioComponent(input, id);
			if (foundAudioComponent != nullptr)
				return foundAudioComponent;
		}
	}
	return nullptr;
}


bool UIToBackendAdapter::idExists(AudioComponent* component, const unsigned int id)
{
	return getAudioComponent(component, id) != nullptr;
}

Node* UIToBackendAdapter::getNode(Node& node, NodeManager& nodeManager, LinkManager& linkManager, const unsigned int audioComponentId)
{
	if (node.audioComponentId == audioComponentId)
		return &node;

	const std::list<LinkInfo> nodeLinks = linkManager.findNodeLinks(nodeManager, node.id, 1);

	for (const LinkInfo& link : nodeLinks) // Loop over links where node is an input
	{
		const std::shared_ptr<Node>& inputNode = nodeManager.findNodeByPinId(link.OutputId);
		assert(inputNode.get());

		if (getNode(*inputNode, nodeManager, linkManager, audioComponentId))
			return inputNode.get();
	}
	return nullptr;
}

Node* UIToBackendAdapter::getNodeDirectChild(Node* node, NodeManager& nodeManager, LinkManager& linkManager, const unsigned int id)
{
	if (!node) return nullptr;

	const std::list<LinkInfo> nodeLinks = linkManager.findNodeLinks(nodeManager, node->id, 1);

	for (const LinkInfo& link : nodeLinks) // Loop over links where node is an input
	{
		const std::shared_ptr<Node>& inputNode = nodeManager.findNodeByPinId(link.OutputId);
		assert(inputNode.get());

		if (inputNode->audioComponentId == id)
			return inputNode.get();
	}

	return nullptr;
}

bool UIToBackendAdapter::idExists(Node& node, NodeManager& nodeManager, LinkManager& linkManager, const unsigned int id)
{
	return getNode(node, nodeManager, linkManager, id) != nullptr;
}

void UIToBackendAdapter::addAudioComponent(AudioComponent* master, Node& masterUI, LinkManager& linkManager, NodeManager& nodeManager, const AddNode& instruction)
{
	//Logger::log("ADD INPUT", Warning) << instruction.UI_NODE_ID << " " << instruction.UI_PARENT_NODE_ID << std::endl;

	// Get UI Node from instruction data
	Node* node = nodeManager.findNodeById(instruction.UI_NODE_ID).get();
	Node* parentNode = nodeManager.findNodeById(instruction.UI_PARENT_NODE_ID).get();

	// If root UI node (id == 1) was just instanciated, set its audioComponentId to 1 which is always master id
	if (parentNode->id == 1 && parentNode->audioComponentId == 0)
		parentNode->audioComponentId = 1;

	// Check if child AudioComponent already exists
	AudioComponent* newAudioComponent = getAudioComponent(master, node->audioComponentId);
	if (newAudioComponent == nullptr)
	{
		// Allocate new AudioComponent and link UI node to it
		newAudioComponent = node->convertNodeToAudioComponent(); assert(newAudioComponent);
		node->audioComponentId = newAudioComponent->id;
	}

	// Link new AudioComponent with its parent
	AudioComponent* parentAudioComponent = getAudioComponent(master, parentNode->audioComponentId); assert(parentAudioComponent);
	parentAudioComponent->addInput(instruction.UI_PARENT_NODE_INPUT_ID - 1, newAudioComponent);
}
