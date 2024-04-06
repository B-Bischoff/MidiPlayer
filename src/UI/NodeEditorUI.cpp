#include "NodeEditorUI.hpp"

int MasterNode::nextId = 0;

Node& findNodeByPinId(std::vector<Node*>& nodes, ed::PinId id)
{
	for (Node* node : nodes)
	{
		for (Pin& pin : node->inputs)
			if (pin.id == id)
				return *node;
		for (Pin& pin : node->outputs)
			if (pin.id == id)
				return *node;
	}

	assert(0 && "[NODE]: Node id does not exits");
}

ed::PinKind getPinKind(ed::PinId pinId, std::vector<Node*>& nodes)
{
	for (Node* node : nodes)
	{
		for (Pin& pin : node->inputs)
		{
			if (pin.id == pinId)
				return ed::PinKind::Input;
		}
		for (Pin& pin : node->outputs)
		{
			if (pin.id == pinId)
				return ed::PinKind::Output;
		}
	}
	assert(0 && "[NODE]: Pin id does not exits");
}

MasterNode& getMasterNode(std::vector<Node*>& nodes)
{
	for (Node* n: nodes)
	{
		MasterNode* masterNode = dynamic_cast<MasterNode*>(n);
		if (masterNode)
			return *masterNode;
	}
	assert(0 && "[NODE]: Master node does not exists");
}

AudioComponent* allocateAudioComponent(UI_NodeType type)
{
//enum UI_NodeType { NodeUI, MasterUI, NumberUI, OscUI, ADSRUI, KbFreqUI };
	switch (type)
	{
		case NodeUI: std::cout << "node" << std::endl; break;
		case MasterUI: std::cout << "master" << std::endl; break;
		case NumberUI: std::cout << "number" << std::endl; break;
		case OscUI: std::cout << "osc" << std::endl; return new Oscillator();
		case ADSRUI: std::cout << "adsr" << std::endl; return new ADSR();
		case KbFreqUI: std::cout << "kb freq" << std::endl; return new KeyboardFrequency();
	}
	assert(0 && "Invalid type");
}

void deleteComponentAndInputs(AudioComponent* component)
{
	if (!component || !component->input)
		return;

	deleteComponentAndInputs(component->input);
	delete component;
}

void createAudioComponentsFromNodes(AudioComponent& component, Node& node, std::vector<Node*>& nodes, ImVector<LinkInfo>& links)
{
	for (Pin& pin : node.inputs)
	{
		for (LinkInfo& link: links)
		{
			if (link.OutputId == pin.id)
			{
				Node& linkedNode = findNodeByPinId(nodes, link.InputId);
				//std::cout << linkedNode.name << " -> " << node.name << std::endl;;
				component.input = allocateAudioComponent(linkedNode.type);
				createAudioComponentsFromNodes(*component.input, linkedNode, nodes, links);
			}
		}
	}
}

// [TODO] keep the function name for different models types (e.g file)
void updateAudioComponents(AudioComponent& master, Node& node, std::vector<Node*>& nodes, ImVector<LinkInfo>& links)
{
	deleteComponentAndInputs(master.input);
	master.input = nullptr;

	createAudioComponentsFromNodes(master, node, nodes, links);
}

NodeEditorUI::NodeEditorUI()
{
	_context = ed::CreateEditor();

	// [TODO] create a node manager storing contiguously nodes in memory
	_nodes.push_back(new MasterNode());
	_nodes.push_back(new NumberNode());
	_nodes.push_back(new OscNode());
	_nodes.push_back(new KeyboardFrequencyNode());
	_nodes.push_back(new ADSR_Node());
}

void NodeEditorUI::update(Master& master)
{
	ed::SetCurrentEditor(_context);
	ed::Begin("Node editor", ImVec2(0.0, 0.0f));

	for (Node* node : _nodes)
		node->render();

	for (auto& linkInfo : _links) // Render links
		ed::Link(linkInfo.Id, linkInfo.InputId, linkInfo.OutputId);

	if (ed::BeginCreate())
	{
		ed::PinId inputPinId, outputPinId;
		if (ed::QueryNewLink(&inputPinId, &outputPinId))
		{
			// Link creation logic

			if (inputPinId && outputPinId && \
				findNodeByPinId(_nodes, inputPinId) != findNodeByPinId(_nodes, outputPinId)) // Do not accept pins from the same node
			{
				if (ed::AcceptNewItem())
				{
					// input & output pins may be reversed depending on the selection order of the nodes link
					if (getPinKind(inputPinId, _nodes) == ed::PinKind::Input && getPinKind(outputPinId, _nodes) == ed::PinKind::Output)
						std::swap(inputPinId, outputPinId);

					// Since we accepted new link, lets add one to our list of links.
					_links.push_back({ ed::LinkId(MasterNode::getNextId()), inputPinId, outputPinId });

					// Draw new link.
					ed::Link(_links.back().Id, _links.back().InputId, _links.back().OutputId);

					updateAudioComponents(master, getMasterNode(_nodes), _nodes, _links);
				}

				// You may choose to reject connection between these nodes
				// by calling ed::RejectNewItem(). This will allow editor to give
				// visual feedback by changing link thickness and color.
			}
		}
	}
	ed::EndCreate();

	// Handle deletion action
	if (ed::BeginDelete())
	{
		// There may be many links marked for deletion, let's loop over them.
		ed::LinkId deletedLinkId;
		while (ed::QueryDeletedLink(&deletedLinkId))
		{
			// If you agree that link can be deleted, accept deletion.
			if (ed::AcceptDeletedItem())
			{
				// Then remove link from your data.
				for (auto& link : _links)
				{
					if (link.Id == deletedLinkId)
					{
						_links.erase(&link);

						updateAudioComponents(master, getMasterNode(_nodes), _nodes, _links);
						break;
					}
				}
			}

			// You may reject link deletion by calling:
			// ed::RejectDeletedItem();
		}
	}
	ed::EndDelete(); // Wrap up deletion action

	ed::End();
}
