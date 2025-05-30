#include "UI/NodeEditorUI.hpp"

bool Node::propertyChanged = false;

NodeEditorUI::NodeEditorUI()
{
	_context = ed::CreateEditor();
	_UIModified = false;
	_navigateToContent = false;

	_nodeManager.registerNode<MasterNode, Master>("Master");
	_nodeManager.registerNode<NumberNode, Number>("Number");
	_nodeManager.registerNode<OscNode, Oscillator>("Oscillator");
	_nodeManager.registerNode<ADSR_Node, ADSR>("ADSR Envelope");
	_nodeManager.registerNode<KeyboardFrequencyNode, KeyboardFrequency>("Keyboard Frequency");
	_nodeManager.registerNode<MultNode, Multiplier>("Multiply");
	_nodeManager.registerNode<LowPassFilterNode, LowPassFilter>("Low Pass Filter");
	_nodeManager.registerNode<HighPassFilterNode, HighPassFilter>("High Pass Filter");
	_nodeManager.registerNode<CombFilterNode, CombFilter>("Comb Filter");
	_nodeManager.registerNode<OverdriveNode, Overdrive>("Overdrive");

	_nodeManager.addNode<MasterNode>(_idManager);

	initStyle();
}

NodeEditorUI::~NodeEditorUI()
{
	ed::DestroyEditor(_context);
}

void NodeEditorUI::initStyle()
{
	ed::SetCurrentEditor(_context);
	ed::Style& style = ed::GetStyle();

	style.NodeBorderWidth = 1.0f;
	style.HoveredNodeBorderWidth = 3.0f;
	style.SelectedNodeBorderWidth = 5.0f;

	constexpr ImVec4 nodeBackground(
		UI_Colors::background_light.x,
		UI_Colors::background_light.y,
		UI_Colors::background_light.z,
		0.75);


	style.Colors[ax::NodeEditor::StyleColor_Grid]              = ImColor(0);
	style.Colors[ax::NodeEditor::StyleColor_NodeBg]            = nodeBackground;
	style.Colors[ax::NodeEditor::StyleColor_PinRect]           = UI_Colors::base;
	style.Colors[ax::NodeEditor::StyleColor_HovNodeBorder]     = UI_Colors::base;
	style.Colors[ax::NodeEditor::StyleColor_SelNodeBorder]     = UI_Colors::highlight;
	style.Colors[ax::NodeEditor::StyleColor_HovLinkBorder]     = UI_Colors::base;
	style.Colors[ax::NodeEditor::StyleColor_SelLinkBorder]     = UI_Colors::highlight;
	style.Colors[ax::NodeEditor::StyleColor_NodeSelRect]       = UI_Colors::light;
	style.Colors[ax::NodeEditor::StyleColor_NodeSelRectBorder] = UI_Colors::base;
}

void NodeEditorUI::update(Master& master, std::queue<Message>& messages, Instrument* selectedInstrument)
{
	ImGuiWindowFlags f;
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

	// Do not open window when no instrument is selected
	const bool windowOpened = ImGui::Begin("Node editor") && (selectedInstrument != nullptr);
	ImGui::PopStyleVar(1);
	if (windowOpened)
	{
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
		ImGui::Text("Create new node");
		ImGui::Separator();

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
		{
			showLabel("Cannot link slider");
			ed::RejectNewItem(ImColor(255, 128, 128), 2.0f);
		}
		else if (inputPin.kind == outputPin.kind)
		{
			if (inputPin.kind == PinKind::Input)
				showLabel("Cannot link two inputs");
			else
				showLabel("Cannot link two outputs");
			ed::RejectNewItem(ImColor(255, 128, 128), 2.0f);
		}
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
			deleteNode(nodeId, messages);
	}
}

void NodeEditorUI::deleteNode(const ed::NodeId& id, std::queue<Message>& messages)
{
	std::shared_ptr<Node>& node = _nodeManager.findNodeById(id);
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

		_linkManager.removeLinksFromNodeId(_idManager, _nodeManager, id);
		_nodeManager.removeNode(_idManager, node);

		messages.push(Message{UI_NODE_DELETED, new unsigned int(id.Get())});
	}
	_UIModified = true;
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

	cereal::JSONInputArchive archive(file);
	_nodeManager.load(archive, _idManager);
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

	cereal::JSONInputArchive archive(stream);
	_nodeManager.load(archive, _idManager);
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
	NodeUIManagers managers = {_nodeManager, _linkManager};
	UIToBackendAdapter::updateBackend(master, managers);
}

void NodeEditorUI::copySelectedNode()
{
	const unsigned int selectedObjectCount = ed::GetSelectedObjectCount();
	std::unique_ptr<ed::NodeId[]> selectedNodes(new ed::NodeId[selectedObjectCount]);
	ed::GetSelectedNodes(selectedNodes.get(), selectedObjectCount);
	unsigned int copiedNodeNumber = 0; // Used by notification

	_copiedNodesInfo = {}; // Reset prevous copied data

	// Copy
	for (int i = 0; i < selectedObjectCount; i++)
	{
		const ed::NodeId& id = selectedNodes[i];
		if (id.Get() == MASTER_NODE_ID)
		{
			ImGui::InsertNotification({ImGuiToastType::Warning, 3000, "Master node cannot be copied"});
			continue; // Do not copy master node
		}

		const std::shared_ptr<Node>& node = _nodeManager.findNodeById(id); assert(node.get());
		const NodeInfo nodeInfo = _nodeManager.getNodeInfo(node.get());
		_copiedNodesInfo.nodes.push_back(std::unique_ptr<Node>(nodeInfo.instantiateCopyFunction(node.get())));

		_copiedNodesInfo.positions.push_back(ed::GetNodePosition(id));
		copiedNodeNumber++;

		const std::list<LinkInfo>& links = _linkManager.findNodeLinks(_nodeManager, id, 1);
		std::list<SavedLinkInfo> savedLinksInfo;
		for (const LinkInfo& link : links)
		{
			if (_nodeManager.findNodeByPinId(link.OutputId)->hidden)
				continue; // Skip hidden nodes (optimization) as they will recreated anyway

			SavedLinkInfo linkInfo;
			linkInfo.outputNodeId = _nodeManager.findNodeByPinId(link.OutputId)->id;
			linkInfo.inputNodeId = _nodeManager.findNodeByPinId(link.InputId)->id;
			linkInfo.inputIndex = _nodeManager.findNodeByPinId(link.InputId)->getInputIndexFromPinId(link.InputId.Get()) - 1;
			savedLinksInfo.push_back(linkInfo);
		}

		_copiedNodesInfo.links.push_back(savedLinksInfo);

		for (int inputIndex = 0; inputIndex < node->inputs.size(); inputIndex++)
		{
			const Pin& pin = node->inputs[inputIndex];

			if (pin.mode == Pin::Mode::Slider)
			{
				float value = *(_nodeManager.findNodeById(id)->inputs[inputIndex].sliderValue);
				_copiedNodesInfo.hiddenNodes.push_back({id, inputIndex, value});
			}
		}
	}
	ImGui::InsertNotification({ImGuiToastType::Info, 3000, "Copied %d nodes to clipboard", copiedNodeNumber});
}

void NodeEditorUI::paste(const ImVec2& cursorPos)
{
	// Associate original node (used as key) with its copy (used as value)
	std::unordered_map<unsigned int, unsigned int> nodeTable;
	ImVec2 origin(0, 0);

	for (int i = 0; i < _copiedNodesInfo.nodes.size(); i++)
	{
		if (i == 0)
			origin = _copiedNodesInfo.positions[0];

		const ed::NodeId& id = _copiedNodesInfo.nodes[i]->id;
		if (id.Get() == MASTER_NODE_ID) continue; // Do not copy master node

		// Copy node
		const std::unique_ptr<Node>& node = _copiedNodesInfo.nodes[i];
		const NodeInfo nodeInfo = _nodeManager.getNodeInfo(node.get());
		Node* nodeCopy = nodeInfo.instantiateCopyFunction(node.get());

		// Update node/pins ids
		nodeCopy->id = _idManager.getID();
		nodeCopy->audioComponentId = 0;
		nodeCopy->initPinsId(_idManager);
		nodeCopy->updatePinsNodePointer();

		nodeTable[node->id] = nodeCopy->id;
		_nodeManager.addNode(nodeCopy);

		// Restore nodes placement from cursor position
		const ImVec2 nodePosition = _copiedNodesInfo.positions[i];
		const ImVec2 delta = ImVec2(nodePosition.x - origin.x, nodePosition.y - origin.y);
		const ImVec2 finalPos = ImVec2(ed::ScreenToCanvas({cursorPos.x, 0}).x + delta.x, ed::ScreenToCanvas({0, cursorPos.y}).y + delta.y);
		ed::SetNodePosition(nodeCopy->id, finalPos);

		// Check for pin linked to hidden nodes
		for (int pinIndex = 0; pinIndex < nodeCopy->inputs.size(); pinIndex++)
		{
			Pin& pin = nodeCopy->inputs[pinIndex];

			if (pin.mode == Pin::Mode::Slider)
			{
				// Create a hidden number node
				std::shared_ptr<Node> hiddenNode = _nodeManager.addNode<NumberNode>(_idManager);
				hiddenNode->hidden = true; // Node and link won't be displayed
				_linkManager.addLink(_idManager, _nodeManager, pin.id, hiddenNode->outputs[0].id);

				// Slider on the current node will control the new hidden number node
				NumberNode* number = (NumberNode*)hiddenNode.get();
				pin.sliderValue = &number->value;

				// Restore number value
				for (const HiddenNodeInfo& hiddenNodeInfo : _copiedNodesInfo.hiddenNodes)
				{
					if (hiddenNodeInfo.nodeId != id || nodeCopy->getInputIndexFromPinId(pin.id) - 1 != hiddenNodeInfo.inputIndex)
						continue;
					number->value = hiddenNodeInfo.value;
					break;
				}
			}
		}
	}

	// Recreate links from original nodes
	for (int i = 0; i < _copiedNodesInfo.nodes.size(); i++)
	{
		const std::list<SavedLinkInfo>& links = _copiedNodesInfo.links[i];

		for (const SavedLinkInfo& link : links)
		{
			auto inputNodeIt = nodeTable.find(link.inputNodeId.Get());
			auto outputNodeIt = nodeTable.find(link.outputNodeId.Get());

			if (inputNodeIt != nodeTable.end() && outputNodeIt != nodeTable.end()) // Link input/output must be in nodeTable
			{
				const std::shared_ptr<Node>& inputNodeCopy = _nodeManager.findNodeById(inputNodeIt->second);
				const std::shared_ptr<Node>& outputNodeCopy = _nodeManager.findNodeById(outputNodeIt->second);
				_linkManager.addLink(_idManager, _nodeManager, inputNodeCopy->inputs[link.inputIndex].id, outputNodeCopy->outputs[0].id);
			}
		}
	}
}

void NodeEditorUI::cut(std::queue<Message>& messages)
{
	copySelectedNode();

	const unsigned int selectedObjectCount = ed::GetSelectedObjectCount();
	std::unique_ptr<ed::NodeId[]> selectedNodes(new ed::NodeId[selectedObjectCount]);
	ed::GetSelectedNodes(selectedNodes.get(), selectedObjectCount);

	for (int i = 0; i < selectedObjectCount; i++)
		deleteNode(selectedNodes[i], messages);

	ed::ClearSelection();
}

void NodeEditorUI::showLabel(const std::string& label) const
{
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ImGui::GetTextLineHeight());
	auto size = ImGui::CalcTextSize(label.c_str());

	auto padding = ImGui::GetStyle().FramePadding;
	auto spacing = ImGui::GetStyle().ItemSpacing;

	ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(spacing.x, -spacing.y));

	auto rectMin = ImGui::GetCursorScreenPos() - padding;
	auto rectMax = ImGui::GetCursorScreenPos() + size + padding;

	auto drawList = ImGui::GetWindowDrawList();
	drawList->AddRectFilled(rectMin, rectMax, ImColor(45, 32, 32, 180), size.y * 0.15f);
	ImGui::TextUnformatted(label.c_str());
}
