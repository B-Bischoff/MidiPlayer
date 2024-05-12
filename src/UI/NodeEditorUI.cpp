#include "UI/NodeEditorUI.hpp"

bool Node::propertyChanged = false;

NodeEditorUI::NodeEditorUI()
{
	_context = ed::CreateEditor();
	_UIModified = false;

	// [TODO] create a node manager storing contiguously nodes in memory
	_nodeManager.addNode<MasterNode>(_idManager);
	_nodeManager.addNode<OscNode>(_idManager);
	_nodeManager.addNode<NumberNode>(_idManager);
}

void NodeEditorUI::update(Master& master)
{
	//if (ImGui::Button("Save Instrument: "))
	//	serialize();
	//if (ImGui::Button("Load Instrument: "))
	//	loadFile(master, "serialization.json");
	static char instrumentFilename[128] = "";
	ImGui::SameLine();
	ImGui::InputText("filename", instrumentFilename, IM_ARRAYSIZE(instrumentFilename));

	ed::SetCurrentEditor(_context);
	ed::Begin("Node editor", ImVec2(0.0, 0.0f));

	render();

	handleCreation(master);
	handleDeletion(master);

	ed::End();

	if (_UIModified || Node::propertyChanged)
	{
		_UIModified = false;
		Node::propertyChanged = false;
		UIToBackendAdapter::updateBackend(master, _nodeManager, _linkManager);
	}
}

void NodeEditorUI::render()
{
	_nodeManager.render();
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
		{
			ed::DeleteNode(contextNodeId);
			/*
			std::cout << "DELETE FROM CONTEXT MENU ON NODE " << contextNodeId.Get() << std::endl;
			std::shared_ptr<Node>& node = _nodeManager.findNodeById(contextNodeId);
			if (node->type != UI_NodeType::MasterUI)
			{
				std::cout << contextNodeId.Get() << std::endl;
				_linkManager.removeLinksFromNodeId(_idManager, _nodeManager, contextNodeId);
				_nodeManager.removeNode(_idManager, contextNodeId);
				_UIModified = true;
			}*/
		}
		ImGui::EndPopup();
	}

	if (ImGui::BeginPopup("Create New Node"))
	{
		std::shared_ptr<Node> node = nullptr;

		if (ImGui::MenuItem("Number"))
			node = _nodeManager.addNode<NumberNode>(_idManager);
		if (ImGui::MenuItem("Oscillator"))
			node = _nodeManager.addNode<OscNode>(_idManager);
		if (ImGui::MenuItem("ADSR Envelope"))
			node = _nodeManager.addNode<ADSR_Node>(_idManager);
		if (ImGui::MenuItem("Keyboard Frequency"))
			node = _nodeManager.addNode<KeyboardFrequencyNode>(_idManager);
		if (ImGui::MenuItem("Multiply"))
			node = _nodeManager.addNode<MultNode>(_idManager);
		if (ImGui::MenuItem("LowPassFilter"))
			node = _nodeManager.addNode<LowPassFilterNode>(_idManager);

		if (node)
		{
			ImVec2 newNodePosition = openPopupPosition;
			_nodeManager.setNodePosition(node, newNodePosition);
			_UIModified = true;
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
			if (ed::AcceptNewItem())
			{
				_linkManager.addLink(_idManager, _nodeManager, inputPinId, outputPinId);
				_UIModified = true;
			}

			// You may choose to reject connection between these nodes
			// by calling ed::RejectNewItem(). This will allow editor to give
			// visual feedback by changing link thickness and color.
		}
	}
	ed::EndCreate();
}

void NodeEditorUI::handleDeletion(Master& master)
{
	if (ed::BeginDelete())
	{
		handleLinkDeletion(master);
		handleNodeDeletion();
	}
	ed::EndDelete();
}

void NodeEditorUI::handleNodeDeletion()
{
	ed::NodeId nodeId = 0;
	while (ed::QueryDeletedNode(&nodeId))
	{
		if (ed::AcceptDeletedItem())
		{
			std::cout << "DELETE FROM HANDLE NODE DEL ON NODE " << nodeId.Get() << std::endl;
			std::shared_ptr<Node>& node = _nodeManager.findNodeById(nodeId);
			if (node->type != UI_NodeType::MasterUI)
			{
				_linkManager.removeLinksFromNodeId(_idManager, _nodeManager, nodeId);
				_nodeManager.removeNode(_idManager, node);
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

	{
		cereal::JSONOutputArchive outputArchive(file);

		/*
		for (auto& node : _nodes)
		{
			outputArchive(
				cereal::make_nvp("node", node),
				cereal::make_nvp("node_position", ed::GetNodePosition(node->id))
			);
		}*/
		for (LinkInfo& link : _links)
			outputArchive(cereal::make_nvp("link", link));
	}
}

void NodeEditorUI::loadFile(Master& master, const fs::path& path)
{
	std::ifstream file(path);
	assert(file.is_open());

	// Clear nodes
	_nodeManager.removeAllNodes(_idManager);

	// Clear links
	for (LinkInfo& link : _links)
		_idManager.releaseID(link.Id.Get());
	_links.clear();

	std::vector<std::shared_ptr<Node>> nodes;
	ImVector<LinkInfo> links;

	{
		cereal::JSONInputArchive archive(file);

		// Load nodes
		try{
			while (true)
			{
				std::shared_ptr<Node> node;
				archive(node);
				ImVec2 pos;
				archive(pos);

				registerNodeIds(*node);
				nodes.push_back(node);

				ed::SetNodePosition(node->id, pos);
			}
		}
		catch (std::exception& e)
		{ }
	}

	file.close();
	file.open(path);

	cereal::JSONInputArchive archive(file);
	archive.setNextName("link");

	try{
		int i = 0;
		while (true)
		{
			archive.startNode();
			int id, inputId, outputId;
			archive(id, inputId, outputId);
			archive.finishNode();

			LinkInfo link = {
				.Id = id,
				.InputId = inputId,
				.OutputId = outputId,
			};
			link.Id = _idManager.getID(link.Id.Get());
			links.push_back(link);
		}
	}
	catch (std::exception& e)
	{ }

	//_nodes = nodes;
	_links = links;

	UIToBackendAdapter::updateBackend(master, _nodeManager, _linkManager);
}

void NodeEditorUI::registerNodeIds(Node& node)
{
	// Register node itself
	node.id = _idManager.getID(node.id);
	assert(node.id != INVALID_ID && "Could not load node");

	// Register node pins
	for (Pin& pin : node.inputs)
	{
		pin.id = _idManager.getID(pin.id);
		assert(pin.id != INVALID_ID && "Could not load node");
	}
	for (Pin& pin : node.outputs)
	{
		pin.id = _idManager.getID(pin.id);
		assert(pin.id != INVALID_ID && "Could not load node");
	}
}
