#include "UI/UIToBackendAdapter.hpp"

void UIToBackendAdapter::updateBackend(Master& master, NodeManager& nodeManager, LinkManager& linkManager)
{
	Components inputs = master.getInputs();
	Node& UIMaster = *nodeManager.getMasterNode();

	printTree(&master);
	printTree(UIMaster, linkManager, nodeManager);

	for (AudioComponent* input : inputs)
		deleteComponentAndInputs(input);

	master.clearInputs();
	updateBackendNode(master, UIMaster, nodeManager, linkManager);
}

void UIToBackendAdapter::deleteComponentAndInputs(AudioComponent* component)
{
	if (!component)
		return;
	Components inputs = component->getInputs();
	if (inputs.empty())
	{
		delete component;
		return;
	}

	for (AudioComponent* input : inputs)
		deleteComponentAndInputs(input);

	delete component;
}

void UIToBackendAdapter::updateBackendNode(AudioComponent& component, Node& node, NodeManager& nodeManager, LinkManager& linkManager)
{
	// Get all links where node is the input
	std::list<LinkInfo> nodeLinks = linkManager.findNodeLinks(nodeManager, node.id, 1);

	for (LinkInfo& link : nodeLinks)
	{
		std::shared_ptr<Node>& inputNode = nodeManager.findNodeByPinId(link.OutputId);
		assert(inputNode);
		AudioComponent* newInput = allocateAudioComponent(*inputNode);

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

		updateBackendNode(*newInput, *inputNode, nodeManager, linkManager);
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

void UIToBackendAdapter::printTree(AudioComponent* component, int depth, int inputIndex, std::vector<bool> drawVertical)
{
	if (depth == 0) std::cout << "================== BACKEND TREE ==================" << std::endl << std::endl;

	drawTree(depth, inputIndex, drawVertical, component->componentName);

	drawVertical.push_back(true); // Assume there are children by default
	int childCount = 0;
	for (const auto& inputs : component->inputs)
		childCount += inputs.size();

	int currentInputIndex = 1;
	int processedChildren = 0;
	for (const auto& inputs : component->inputs) // loop over component inputs
	{
		for (const auto& input : inputs) // loop over all the component plugged to one input
		{
			processedChildren++;
			if (processedChildren == childCount)
				drawVertical.back() = false; // Stop drawing vertical for the last child

			printTree(input, depth + 1, currentInputIndex, drawVertical);
		}
		currentInputIndex++;
	}
}

void UIToBackendAdapter::printTree(Node& node, LinkManager& linkManager, NodeManager& nodeManager, int depth, int inputIndex, std::vector<bool> drawVertical)
{
	if (depth == 0) std::cout << "================== UI NODES TREE ==================" << std::endl << std::endl;

	drawTree(depth, inputIndex, drawVertical, node.name + " id " + std::to_string(node.id));

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
