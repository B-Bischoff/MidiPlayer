#include "UI/NodeEditorUI.hpp"
#include "imgui_node_editor.h"

bool Node::propertyChanged = false;

NodeEditorUI::NodeEditorUI()
{
	_context = ed::CreateEditor();
	_UIModified = false;
	_navigateToContent = false;

	_nodeManager.registerNode<MasterNode>("Master");
	_nodeManager.registerNode<NumberNode>("Number");
	_nodeManager.registerNode<OscNode>("Oscillator");
	_nodeManager.registerNode<ADSR_Node>("ADSR Envelope");
	_nodeManager.registerNode<KeyboardFrequencyNode>("Keyboard Frequency");
	_nodeManager.registerNode<MultNode>("Multiply");
	_nodeManager.registerNode<LowPassFilterNode>("Low Pass Filter");
	_nodeManager.registerNode<CombFilterNode>("Comb Filter");

	_nodeManager.addNode<MasterNode>(_idManager);
}

NodeEditorUI::~NodeEditorUI()
{
	ed::DestroyEditor(_context);
}

void NodeEditorUI::update(Master& master, std::queue<Message>& messages, Instrument* selectedInstrument)
{
	if (!ImGui::Begin("Node editor"))
		return;

	if (!selectedInstrument)
	{
		// [TODO] print a warning message ?
		ImGui::End();
		return;
	}

	ed::SetCurrentEditor(_context);
	ed::Begin("Node editor", ImVec2(0, 0));

	render(messages);

	handleCreation(master);
	handleDeletion(master, messages);

	ed::End();

	if (_UIModified || Node::propertyChanged)
		updateBackend(master);

	if (_navigateToContent)
	{
		_navigateToContent = false;
		ed::NavigateToContent();
	}

	ImGui::End();
}

void NodeEditorUI::render(std::queue<Message>& messages)
{
	_nodeManager.render(messages);
	_linkManager.render();
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
	static ed::PinId contextPinId = 0;

	ed::Suspend();
	if (ed::ShowBackgroundContextMenu())
		ImGui::OpenPopup("Create New Node");
	else if (ed::ShowNodeContextMenu(&contextNodeId))
		ImGui::OpenPopup("Node Context Menu");
	else if (ed::ShowPinContextMenu(&contextPinId))
		ImGui::OpenPopup("Pin Context Menu");
	ed::Resume();

	ed::Suspend();

	if (ImGui::BeginPopup("Create New Node"))
	{
		std::shared_ptr<Node> node = nullptr;

		// Loop over all the registered node and print their name to the menu item
		for (const auto& info : _nodeManager._nodesInfo)
		{
			// Do not allow another master node creation
			if (info.second.name == "Master") continue;

			if (ImGui::MenuItem(info.second.name.c_str()))
				node = _nodeManager.addNode(info.second.instantiateFunction(&_idManager));
		}

		if (node)
		{
			ImVec2 newNodePosition = openPopupPosition;
			_nodeManager.setNodePosition(node, newNodePosition);
			_UIModified = true;
		}

		ImGui::EndPopup();
	}

	if (ImGui::BeginPopup("Node Context Menu"))
	{
		ImGui::TextUnformatted("Node Context Menu");
		ImGui::Separator();
		if (ImGui::MenuItem("Delete"))
			ed::DeleteNode(contextNodeId);
		ImGui::EndPopup();
	}

	if (ImGui::BeginPopup("Pin Context Menu"))
	{
		Pin& pin = _nodeManager.findPinById(contextPinId);

		ImGui::Text("Pin id : %d", pin.id);

		if (pin.kind == PinKind::Input)
		{
			int type = pin.mode;
			ImGui::Text("Input type : ");
			ImGui::SameLine();
			ImGui::RadioButton("node link", &type, 0); ImGui::SameLine();
			ImGui::RadioButton("slider", &type, 1);

			if (type != pin.mode)
			{
				pin.mode = static_cast<Pin::Mode>(type);

				if (pin.mode == Pin::Mode::Link)
				{
					// Delete linked hidden number node
					std::list<LinkInfo> links = _linkManager.findPinLinks(pin.id);
					assert(links.size() == 1);
					std::shared_ptr<Node> hiddenNode = _nodeManager.findNodeByPinId(links.begin()->OutputId.Get());

					_linkManager.removeLinksFromNodeId(_idManager, _nodeManager, hiddenNode->id);
					_nodeManager.removeNode(_idManager, hiddenNode);
				}
				else
				{
					// Remove existing pin links
					_linkManager.removeLinksContainingPinId(_idManager, pin.id);

					// Create a hidden number node
					std::shared_ptr<Node> node = _nodeManager.addNode<NumberNode>(_idManager);
					node->hidden = true; // Node and link won't be displayed
					_linkManager.addLink(_idManager, _nodeManager, pin.id, node->outputs[0].id);

					// Slider on the current node will control the new hidden number node
					NumberNode* number = (NumberNode*)node.get();
					pin.sliderValue = &number->value;
				}

				_UIModified = true;
			}
		}

		ImGui::EndPopup();
	}

	ed::Resume();
}

void NodeEditorUI::handleLinkCreation(Master& master)
{
	if (!ed::BeginCreate())
		return;

	ed::PinId inputPinId, outputPinId;
	if (ed::QueryNewLink(&inputPinId, &outputPinId))
	{
		Pin& inputPin = _nodeManager.findPinById(inputPinId);
		Pin& outputPin = _nodeManager.findPinById(outputPinId);

		// Do not accept link on pin using slider
		if (inputPin.mode == Pin::Mode::Slider || outputPin.mode == Pin::Mode::Slider)
			ed::RejectNewItem();
		else if (ed::AcceptNewItem())
		{
			_linkManager.addLink(_idManager, _nodeManager, inputPinId, outputPinId);
			_UIModified = true;
		}

		// You may choose to reject connection between these nodes
		// by calling ed::RejectNewItem(). This will allow editor to give
		// visual feedback by changing link thickness and color.
	}
	ed::EndCreate();
}

void NodeEditorUI::handleDeletion(Master& master, std::queue<Message>& messages)
{
	if (ed::BeginDelete())
	{
		handleLinkDeletion(master);
		handleNodeDeletion(messages);
	}
	ed::EndDelete();
}

void NodeEditorUI::handleNodeDeletion(std::queue<Message>& messages)
{
	ed::NodeId nodeId = 0;
	while (ed::QueryDeletedNode(&nodeId))
	{
		if (ed::AcceptDeletedItem())
		{
			std::shared_ptr<Node>& node = _nodeManager.findNodeById(nodeId);
			if (node->type != UI_NodeType::MasterUI)
			{
				// Delete hidden nodes linked to pin in Slider mode
				for (Pin& pin : node->inputs)
				{
					if (pin.mode == Pin::Mode::Link) continue;

					std::list<LinkInfo> links = _linkManager.findPinLinks(pin.id);
					assert(links.size() == 1);
					std::shared_ptr<Node>& hiddenNode = _nodeManager.findNodeByPinId(links.begin()->OutputId);
					_nodeManager.removeNode(_idManager, hiddenNode);
				}

				_linkManager.removeLinksFromNodeId(_idManager, _nodeManager, nodeId);
				_nodeManager.removeNode(_idManager, node);

				messages.push(Message{UI_NODE_DELETED, new unsigned int(nodeId.Get())});
			}
			_UIModified = true;
		}
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
			_linkManager.removeLink(_idManager, deletedLinkId);
			_UIModified = true;
		}
		// You may reject link deletion by calling:
		// ed::RejectDeletedItem();
	}
}

void NodeEditorUI::serialize(const fs::path& path)
{
	std::ofstream file(path);
	assert(file.is_open() && "[SERIALIZE] could not open file");

	cereal::JSONOutputArchive outputArchive(file);

	_nodeManager.serialize(outputArchive);
	_linkManager.serialize(outputArchive);
}

void NodeEditorUI::serialize(std::stringstream& stream)
{
	cereal::JSONOutputArchive outputArchive(stream);

	_nodeManager.serialize(outputArchive);
	_linkManager.serialize(outputArchive);
}

void NodeEditorUI::loadFile(Master& master, const fs::path& path)
{
	std::ifstream file(path);
	assert(file.is_open());

	_nodeManager.removeAllNodes(_idManager);
	_linkManager.removeAllLinks(_idManager);

	ImVector<LinkInfo> links;

	{
		cereal::JSONInputArchive archive(file);
		_nodeManager.load(archive, _idManager);
	}

	file.close();
	file.open(path);

	cereal::JSONInputArchive archive(file);
	archive.setNextName("link");
	_linkManager.load(archive, _idManager);

	// Link hidden node
	std::list<std::shared_ptr<Node>> hiddenNodes = _nodeManager.getHiddenNodes();
	for (auto& node : hiddenNodes)
	{
		std::list<LinkInfo> links = _linkManager.findNodeLinks(_nodeManager, node->id);
		assert(links.size() == 1);
		Pin& inputPin = _nodeManager.findPinById(links.begin()->InputId);
		inputPin.mode = Pin::Mode::Slider;
		NumberNode* number = (NumberNode*)node.get();
		inputPin.sliderValue = &number->value;
	}

	_UIModified = true;
	_navigateToContent = true;
}

void NodeEditorUI::loadFile(Master& master, std::stringstream& stream)
{
	_nodeManager.removeAllNodes(_idManager);
	_linkManager.removeAllLinks(_idManager);

	ImVector<LinkInfo> links;

	{
		std::stringstream streamCopy1(stream.str());
		cereal::JSONInputArchive archive(streamCopy1);
		_nodeManager.load(archive, _idManager);
	}

	std::stringstream streamCopy2(stream.str());
	cereal::JSONInputArchive archive(streamCopy2);
	archive.setNextName("link");
	_linkManager.load(archive, _idManager);

	// Link hidden node
	std::list<std::shared_ptr<Node>> hiddenNodes = _nodeManager.getHiddenNodes();
	for (auto& node : hiddenNodes)
	{
		std::list<LinkInfo> links = _linkManager.findNodeLinks(_nodeManager, node->id);
		assert(links.size() == 1);
		Pin& inputPin = _nodeManager.findPinById(links.begin()->InputId);
		inputPin.mode = Pin::Mode::Slider;
		NumberNode* number = (NumberNode*)node.get();
		inputPin.sliderValue = &number->value;
	}

	_UIModified = true;
	_navigateToContent = true;
}

void NodeEditorUI::updateBackend(Master& master)
{
	_UIModified = false;
	Node::propertyChanged = false;
	UIToBackendAdapter::updateBackend(master, _nodeManager, _linkManager);
}
