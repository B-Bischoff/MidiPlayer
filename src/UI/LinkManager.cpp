#include "UI/LinkManager.hpp"

void LinkManager::addLink(IDManager& idManager, NodeManager& nodeManager, ed::PinId input, ed::PinId output)
{
	Pin& inputPin = nodeManager.findPinById(input);
	Pin& outputPin = nodeManager.findPinById(output);
	if (inputPin.kind == outputPin.kind)
	{
		std::cout << "[WARNING | LinkManager] : do not link same pin kind." << std::endl;
		return;
	}

	std::shared_ptr<Node>& inputNode = nodeManager.findNodeByPinId(input);
	std::shared_ptr<Node>& outputNode = nodeManager.findNodeByPinId(output);;
	if (inputNode == outputNode)
	{
		std::cout << "[WARNING | LinkManager] : do not link multiple pins from the same node." << std::endl;
		return;
	}

	// Make sure input and output are not reversed
	if (inputPin.kind == PinKind::Output && outputPin.kind == PinKind::Input)
		std::swap(input, output);

	LinkInfo link;
	link.Id = idManager.getID();
	link.InputId = input;
	link.OutputId = output;

	_links.push_back(link);
}

void LinkManager::eraseLink(ed::LinkId id)
{
	for (auto it = _links.begin(); it != _links.end(); it++)
	{
		if (it->Id == id)
		{
			_links.erase(it);
			return;
		}
	}

	assert(0 && "[LinkManager] eraseLink must not be called on non-existing link id.");
}

void LinkManager::releaseLinkId(IDManager& idManager, ed::LinkId id)
{
	idManager.releaseID(id.Get());
}

void LinkManager::removeLink(IDManager& idManager, ed::LinkId id)
{
	releaseLinkId(idManager, id);
	eraseLink(id);
}

void LinkManager::removeAllLinks(IDManager& idManager)
{
	while (_links.size())
		removeLink(idManager, _links.back().Id);
}

void LinkManager::removeLinksFromNodeId(IDManager& idManager, NodeManager& nodeManager, ed::NodeId id)
{
	std::shared_ptr<Node>& node = nodeManager.findNodeById(id);

	for (Pin& pin : node->inputs)
		removeLinksContainingPinId(idManager, pin.id);
	for (Pin& pin : node->outputs)
		removeLinksContainingPinId(idManager, pin.id);
}

void LinkManager::removeLinksContainingPinId(IDManager& idManager, ed::PinId id)
{
	auto it = _links.begin();
	while (it != _links.end())
	{
		if (it->InputId == id || it->OutputId == id)
		{
			removeLink(idManager, it->Id);
			it = _links.begin();
		}
		else
			it++;
	}
}

std::list<LinkInfo> LinkManager::findNodeLinks(NodeManager& nodeManager, ed::NodeId id, int filter)
{
	std::list<LinkInfo> links;

	std::shared_ptr<Node>& node = nodeManager.findNodeById(id);

	for (Pin& pin : node->inputs)
		links.splice(links.end(), findPinLinks(pin.id, filter));
	for (Pin& pin : node->outputs)
		links.splice(links.end(), findPinLinks(pin.id, filter));

	return links;
}

std::list<LinkInfo> LinkManager::findPinLinks(ed::PinId id, int filter)
{
	std::list<LinkInfo> links;
	for (LinkInfo& link : _links)
	{
		// links cannot be added on both conditions because links can't be plug to the same node
		if (link.InputId == id && filter != 2)
			links.push_back(link);
		if (link.OutputId == id && filter != 1)
			links.push_back(link);
	}
	return links;
}

void LinkManager::render()
{
	for (LinkInfo& link : _links)
		ed::Link(link.Id, link.InputId, link.OutputId);
}
