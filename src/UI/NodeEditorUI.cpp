#include "UI/NodeEditorUI.hpp"

bool Node::propertyChanged = false;

NodeEditorUI::NodeEditorUI()
{
	_context = ed::CreateEditor();
	_UIModified = false;

	_nodeManager.addNode<MasterNode>(_idManager);
	_nodeManager.addNode<OscNode>(_idManager);
	_nodeManager.addNode<NumberNode>(_idManager);
}

void NodeEditorUI::update(Master& master)
{
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
			ed::DeleteNode(contextNodeId);
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

	_UIModified = true;
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

	_UIModified = true;
}
