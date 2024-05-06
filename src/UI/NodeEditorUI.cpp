#include "NodeEditorUI.hpp"

bool Node::propertyChanged = false;
int Node::nextId = 0;

Node& findNoteById(std::vector<std::shared_ptr<Node>>& nodes, ed::NodeId id)
{
	for (std::shared_ptr<Node>& node : nodes)
	{
		if (node->id == id)
			return *node;
	}
	assert(0 && "[NODE]: Could not find node by id");
	return *nodes[0];
}

std::vector<std::shared_ptr<Node>>::iterator removeNode(std::vector<std::shared_ptr<Node>>& nodes, Node& nodeToDelete)
{
	for (auto it = nodes.begin(); it != nodes.end(); it++)
	{
		Node* node = it->get();
		assert(node);

		if (node->id == nodeToDelete.id)
			return nodes.erase(it);
	}
	assert(0 && "[NODE]: Could not erase node");
	return nodes.begin();
}

Node& findNodeByPinId(std::vector<std::shared_ptr<Node>>& nodes, ed::PinId id)
{
	for (std::shared_ptr<Node>& node : nodes)
	{
		for (Pin& pin : node->inputs)
			if (pin.id == id)
				return *node;
		for (Pin& pin : node->outputs)
			if (pin.id == id)
				return *node;
	}

	assert(0 && "[NODE]: Node id does not exits");
	return *nodes[0];
}

void removeLinkContainingId(ImVector<LinkInfo>& links, std::vector<std::shared_ptr<Node>>& nodes, ed::NodeId id)
{
	for (auto it = links.begin(); it != links.end(); it++)
	{
		LinkInfo& link = *it;
		Node& inputNode = findNodeByPinId(nodes, link.InputId);
		Node& outputNode = findNodeByPinId(nodes, link.InputId);

		if (inputNode.id == id || outputNode.id == id)
			it = links.erase(it);

		if (it == links.end())
			break;
	}
}

ed::PinKind getPinKind(ed::PinId pinId, std::vector<std::shared_ptr<Node>>& nodes)
{
	for (std::shared_ptr<Node>& node : nodes)
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
	return ed::PinKind::Output;
}

MasterNode& getMasterNode(std::vector<std::shared_ptr<Node>>& nodes)
{
	for (std::shared_ptr<Node>& node : nodes)
	{
		MasterNode* masterNode = dynamic_cast<MasterNode*>(node.get());
		if (masterNode)
			return *masterNode;
	}
	assert(0 && "[NODE]: Master node does not exists");
	return *(MasterNode*)nodes[0].get();
}

AudioComponent* allocateAudioComponent(Node& node)
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
	}
	assert(0 && "Invalid type");
	return nullptr;
}

void deleteComponentAndInputs(AudioComponent* component)
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

void createAudioComponentsFromNodes(AudioComponent& component, Node& outputNode, std::vector<std::shared_ptr<Node>>& nodes, ImVector<LinkInfo>& links)
{
	for (Pin& pin : outputNode.inputs) // Loop through current node input pins
	{
		for (LinkInfo& link: links) // Check for every link that points to the current node input
		{
			if (link.OutputId == pin.id)
			{
				Node& inputNode = findNodeByPinId(nodes, link.InputId); // Get node plugged to the input pin

				AudioComponent* newInput = allocateAudioComponent(inputNode); // Create an AudioComponent version of that node
				component.addInput(pin.name, newInput); // Plug this node into the current node input

				createAudioComponentsFromNodes(*newInput, inputNode, nodes, links); // Recursively follow input nodes
			}
		}
	}
}

// [TODO] keep the function name for different models types (e.g file)
void updateAudioComponents(AudioComponent& master, Node& node, std::vector<std::shared_ptr<Node>>& nodes, ImVector<LinkInfo>& links)
{
	Components inputs = master.getInputs();
	for (AudioComponent* input : inputs)
		deleteComponentAndInputs(input);

	master.clearInputs();

	createAudioComponentsFromNodes(master, node, nodes, links);
}

NodeEditorUI::NodeEditorUI()
{
	_context = ed::CreateEditor();

	// [TODO] create a node manager storing contiguously nodes in memory
	_nodes.push_back(std::make_shared<MasterNode>());
	_nodes.push_back(std::make_shared<OscNode>());
	_nodes.push_back(std::make_shared<NumberNode>());
}

void NodeEditorUI::update(Master& master)
{
	if (ImGui::Button("Save Instrument: "))
		serialize();
	static char instrumentFilename[128] = "";
	ImGui::SameLine();
	ImGui::InputText("filename", instrumentFilename, IM_ARRAYSIZE(instrumentFilename));

	ed::SetCurrentEditor(_context);
	ed::Begin("Node editor", ImVec2(0.0, 0.0f));

	render();

	handleCreation(master);
	handleDeletion(master);

	ed::End();

	if (Node::propertyChanged)
	{
		std::cout << "Property changed" << std::endl;
		Node::propertyChanged = false;
		updateAudioComponents(master, getMasterNode(_nodes), _nodes, _links);
	}
}

void NodeEditorUI::render()
{
	for (std::shared_ptr<Node>& node : _nodes)
		node->render();

	for (auto& linkInfo : _links) // Render links
		ed::Link(linkInfo.Id, linkInfo.InputId, linkInfo.OutputId);
}

void NodeEditorUI::handleCreation(Master& master)
{
	handleNodeCreation();
	handleLinkCreation(master);
}

void NodeEditorUI::handleNodeCreation()
{
	ImVec2 openPopupPosition = ImGui::GetMousePos();
	static Pin* newNodeLinkPin = nullptr; // [TODO] remove those static variables
	static ed::NodeId contextNodeId = 0;

	ed::Suspend();
	if (ed::ShowBackgroundContextMenu())
		ImGui::OpenPopup("Create New Node");
	else if (ed::ShowNodeContextMenu(&contextNodeId))
		ImGui::OpenPopup("Node Context Menu");
	ed::Resume();

	ed::Suspend();

	if (ImGui::BeginPopup("Node Context Menu"))
	{
		ImGui::TextUnformatted("Node Context Menu");
		ImGui::Separator();
		if (ImGui::MenuItem("Delete"))
			removeNodeAndDependencies(contextNodeId);
		ImGui::EndPopup();
	}

	if (ImGui::BeginPopup("Create New Node"))
	{
		Node* node = nullptr;

		if (ImGui::MenuItem("Number"))
			node = addNode<NumberNode>();
		if (ImGui::MenuItem("Oscillator"))
			node = addNode<OscNode>();
		if (ImGui::MenuItem("ADSR Envelope"))
			node = addNode<ADSR_Node>();
		if (ImGui::MenuItem("Keyboard Frequency"))
			node = addNode<KeyboardFrequencyNode>();
		if (ImGui::MenuItem("Multiply"))
			node = addNode<MultNode>();

		if (node)
		{
			ImVec2 newNodePosition = openPopupPosition;
			ed::SetNodePosition(node->id, newNodePosition);
		}

		ImGui::EndPopup();
	}
	ed::Resume();
}

void NodeEditorUI::handleLinkCreation(Master& master)
{
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
}

void NodeEditorUI::handleDeletion(Master& master)
{
	if (ed::BeginDelete())
	{
		handleNodeDeletion();
		handleLinkDeletion(master);
	}
	ed::EndDelete();
}

void NodeEditorUI::handleNodeDeletion()
{
	ed::NodeId nodeId = 0;
	while (ed::QueryDeletedNode(&nodeId))
	{
		if (ed::AcceptDeletedItem())
			removeNodeAndDependencies(nodeId);
	}
}

void NodeEditorUI::handleLinkDeletion(Master& master)
{
	// There may be many links marked for deletion, let's loop over them.
	ed::LinkId deletedLinkId;
	while (ed::QueryDeletedLink(&deletedLinkId))
	{
		// If you agree that link can be deleted, accept deletion.
		if (ed::AcceptDeletedItem())
		{
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

template<typename T>
Node* NodeEditorUI::addNode()
{
	std::shared_ptr<Node> node = std::make_shared<T>();
	_nodes.push_back(node);
	return node.get();
}

void NodeEditorUI::removeNodeAndDependencies(ed::NodeId nodeId)
{
	for (std::shared_ptr<Node>& node : _nodes)
	{
		if (node->id != nodeId)
			continue;

		if (node->type == UI_NodeType::MasterUI)
			return; // [TODO] Display some "Cannot erase master node" message

		removeLinkContainingId(_links, _nodes, nodeId);
		removeNode(_nodes, findNoteById(_nodes, nodeId));
		ed::DeleteNode(nodeId);
		Node::propertyChanged = true;
		return;
	}
}

void NodeEditorUI::serialize()
{
	std::ofstream file("serialization.json");

	{
		/*
		cereal::JSONOutputArchive outputArchive(file);
		for (std::shared_ptr<Node>& node : _nodes)
			outputArchive(*node);
		for (LinkInfo& link : _links)
			outputArchive(link);
			*/
	}
}
