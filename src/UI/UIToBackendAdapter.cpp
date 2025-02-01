#include "UI/UIToBackendAdapter.hpp"

void UIToBackendAdapter::updateBackend(Master& master, NodeManager& nodeManager, LinkManager& linkManager)
{
	Components inputs = master.getInputs();
	Node& UIMaster = *nodeManager.getMasterNode();

	printTreesDiff(&master, UIMaster, linkManager, nodeManager);
	for (AudioComponent* input : inputs)
		deleteComponentAndInputs(input, (AudioComponent*)&master);

	//master.clearInputs();

	updateBackendNode(master, master, UIMaster, nodeManager, linkManager);
	//printTree(&master);
	//printTree(UIMaster, linkManager, nodeManager);
}

// [TODO] This belongs to instruments class
void UIToBackendAdapter::removeComponentFromBackend(AudioComponent* component, AudioComponent* componentToRemove, unsigned int depth)
{
	Components inputs = component->getInputs();
	for (AudioComponent* input : inputs)
		removeComponentFromBackend(input, componentToRemove, depth + 1);

	component->removeInput(componentToRemove);

	if (depth == 0)
	{
		Logger::log("DELETE") << componentToRemove->componentName << " " << componentToRemove->id << std::endl;
		delete componentToRemove;
	}
}

// [TODO] This belongs to instruments class
void UIToBackendAdapter::deleteComponentAndInputs(AudioComponent* component, AudioComponent* master)
{
	Components inputs = component->getInputs();

	auto it = inputs.begin();
	while (it != inputs.end())
	{
		deleteComponentAndInputs(*it, master);
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

		updateBackendNode(master, *newInput, *inputNode, nodeManager, linkManager);
	}
}

/*
void createAudioComponentsFromNodes(AudioComponent& component, Node& outputNode, std::vector<std::shared_ptr<Node>>& nodes, ImVector<LinkInfo>& links)
{
	for (Pin& pin : outputNode.inputs) // Loop through current node input pins
	{
		for (LinkInfo& link: links) // Check for every link that points to the current node input
		{
			if (link.OutputId.Get() == pin.id)
			{
				Node& inputNode = findNodeByPinId(nodes, link.InputId); // Get node plugged to the input pin

				AudioComponent* newInput = allocateAudioComponent(inputNode); // Create an AudioComponent version of that node
				component.addInput(pin.name, newInput); // Plug this node into the current node input

				createAudioComponentsFromNodes(*newInput, inputNode, nodes, links); // Recursively follow input nodes
			}
		}
	}
}
*/

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
	printUIDiff(component, node, linkManager, nodeManager);
	printBackendDiff(component, node, linkManager, nodeManager);
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

void UIToBackendAdapter::printTree(const Node& node, LinkManager& linkManager, NodeManager& nodeManager, int depth, int inputIndex, std::vector<bool> drawVertical)
{
	if (depth == 0) std::cout << "================== UI NODES TREE ==================" << std::endl << std::endl;

	drawTree(depth, inputIndex, drawVertical, node.name + " id: " + std::to_string(node.id) + " | comp id: " + std::to_string(node.audioComponentId));

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

		printTree(*inputNode, linkManager, nodeManager, depth + 1, currentInputIndex, drawVertical);
	}
}

void UIToBackendAdapter::drawTree(int depth, int inputIndex, std::vector<bool> drawVertical, const std::string& text)
{
	for (int i = 0; i < depth; i++)
	{
		if (i == depth - 1 && drawVertical[i])
			std::cout << "├─" << inputIndex << "─";
		else if (i == depth - 1)
			std::cout << "└─" << inputIndex << "─";
		else if (drawVertical[i])
			std::cout << "│   ";
		else
			std::cout << "    ";
	}
	std::cout << text << std::endl;
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

Node* UIToBackendAdapter::getNode(Node& node, NodeManager& nodeManager, LinkManager& linkManager, const unsigned int id)
{
	if (node.audioComponentId == id)
		return &node;

	const std::list<LinkInfo> nodeLinks = linkManager.findNodeLinks(nodeManager, node.id, 1);

	for (const LinkInfo& link : nodeLinks) // Loop over links where node is an input
	{
		const std::shared_ptr<Node>& inputNode = nodeManager.findNodeByPinId(link.OutputId);
		assert(inputNode.get());

		if (getNode(*inputNode, nodeManager, linkManager, id))
			return inputNode.get();
	}
	return nullptr;
}

bool UIToBackendAdapter::idExists(Node& node, NodeManager& nodeManager, LinkManager& linkManager, const unsigned int id)
{
	return getNode(node, nodeManager, linkManager, id) != nullptr;
}
