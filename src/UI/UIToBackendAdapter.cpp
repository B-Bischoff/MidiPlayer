#include "UI/UIToBackendAdapter.hpp"
#include <csetjmp>

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
	std::cout << "updating backend" << std::endl;

	// Get all links where node is the output
	std::list<LinkInfo> nodeLinks = linkManager.findNodeLinks(nodeManager, node.id, 1);
	std::cout << "link size : " << nodeLinks.size() << std::endl;

	for (LinkInfo& link : nodeLinks)
	{
		std::cout << "link info " << link.InputId.Get() << " " <<  link.OutputId.Get() << std::endl;

		std::shared_ptr<Node>& inputNode = nodeManager.findNodeByPinId(link.OutputId);
		std::cout << inputNode->name << std::endl;
		assert(inputNode);
		std::cout << inputNode->type << std::endl;
		AudioComponent* newInput = allocateAudioComponent(*inputNode);

		// Find input name
		std::string inputName;
		for (Pin& pin : node.inputs)
		{
			if (pin.id == link.InputId.Get())
				inputName = pin.name;
		}

		std::cout << node.name << " <- " << inputNode->name << " " << inputName << std::endl;

		component.addInput(inputName, newInput);

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
	UI_NodeType type = node.type;

	switch (type)
	{
		case NodeUI: break;
		case MasterUI: break;
		case NumberUI: {
			NumberNode* numberNode = dynamic_cast<NumberNode*>(&node);
			assert(numberNode);
			Number* number = new Number(); // [TODO] create adequate constructor
			number->number = numberNode->value;
			return number;
		}
		case OscUI: {
			OscNode* oscUI = dynamic_cast<OscNode*>(&node);
			assert(oscUI);
			Oscillator* osc = new Oscillator();
			osc->type = oscUI->oscType;
			return osc;
		}
		case ADSRUI: return new ADSR();
		case KbFreqUI: return new KeyboardFrequency();
		case MultUI: return new Multiplier();
		case LowPassUI: return new LowPassFilter();
	}
	assert(0 && "Invalid type");
	return nullptr;
}
