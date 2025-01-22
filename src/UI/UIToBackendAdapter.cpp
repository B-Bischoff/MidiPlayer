#include "UI/UIToBackendAdapter.hpp"

void UIToBackendAdapter::updateBackend(Master& master, NodeManager& nodeManager, LinkManager& linkManager)
{
	Components inputs = master.getInputs();
	for (AudioComponent* input : inputs)
		deleteComponentAndInputs(input);

	master.clearInputs();

	Node& UIMaster = *nodeManager.getMasterNode();

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
