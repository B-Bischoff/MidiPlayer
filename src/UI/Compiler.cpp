#include "UI/Compiler.hpp"

void Compiler::compile(Instrument& instrument, NodeManager& nodeManager, LinkManager& linkManager)
{
	const auto& masterNode = nodeManager.getMasterNode();
	masterNode->audioComponentId = instrument.master.id;
	masterNode->audioComponent = &instrument.master;

	std::unordered_set<unsigned int> activeNodes;
	std::unordered_set<unsigned int> activePolyKeys;

	// Clear Master's backend inputs — will be rebuilt from UI links
	instrument.master.clearInputs();

	// Get UI nodes connected to Master's input pins
	auto masterInputs = getNodeInputs(masterNode, nodeManager, linkManager);

	for (size_t pinIdx = 0; pinIdx < masterInputs.size(); pinIdx++)
	{
		if (pinIdx >= masterNode->inputs.size()) break;
		int inputId = masterNode->inputs[pinIdx].inputId;
		if (inputId < 0) continue;

		for (auto& inputNode : masterInputs[pinIdx])
		{
			auto component = compileNode(inputNode, nodeManager, linkManager, activeNodes);
			if (!component) continue;

			// If sub-tree contains a MIDI source, wrap in Polyphony
			if (containsMidiSource(component))
			{
				unsigned int polyKey = inputNode->id;
				activePolyKeys.insert(polyKey);

				auto polyIt = _polyMap.find(polyKey);
				std::shared_ptr<Polyphony> poly;

				if (polyIt != _polyMap.end())
				{
					// Reuse existing Polyphony (preserves voice state)
					poly = polyIt->second;
					poly->clearInputs();
				}
				else
				{
					poly = std::make_shared<Polyphony>();
					_polyMap[polyKey] = poly;
				}

				poly->midiSource = findMidiSource(component);
				poly->addInput(Polyphony::audioTemplate, component);
				instrument.master.addInput(inputId, poly);
			}
			else
			{
				instrument.master.addInput(inputId, component);
			}
		}
	}

	// Remove stale entries (UI nodes no longer in the graph)
	for (auto it = _nodeMap.begin(); it != _nodeMap.end(); )
	{
		if (!activeNodes.count(it->first))
			it = _nodeMap.erase(it);
		else
			++it;
	}

	// Remove stale Polyphony nodes
	for (auto it = _polyMap.begin(); it != _polyMap.end(); )
	{
		if (!activePolyKeys.count(it->first))
			it = _polyMap.erase(it);
		else
			++it;
	}

	// Debug: print the compiled backend tree
	for (auto& slot : instrument.master.inputs)
		for (auto& child : slot)
			debugPrintTree(child);
}

std::shared_ptr<AudioComponent> Compiler::compileNode(
	const std::shared_ptr<Node>& uiNode,
	NodeManager& nodeManager,
	LinkManager& linkManager,
	std::unordered_set<unsigned int>& activeNodes)
{
	if (!uiNode || uiNode->type == MasterUI)
		return nullptr;

	// Already compiled this pass? Return existing (handles fan-out / shared nodes)
	if (activeNodes.count(uiNode->id))
	{
		auto it = _nodeMap.find(uiNode->id);
		return (it != _nodeMap.end()) ? it->second : nullptr;
	}
	activeNodes.insert(uiNode->id);

	// Look up existing AudioComponent or create a new one
	std::shared_ptr<AudioComponent> component;
	auto it = _nodeMap.find(uiNode->id);

	if (it != _nodeMap.end())
	{
		// Reuse existing component (preserves audio state: filter coefficients, oscillator phase, etc.)
		component = it->second;

		// Update parameters if they changed in the UI
		if (!(*uiNode == component.get()))
			uiNode->assignToAudioComponent(component.get());
	}
	else
	{
		// Create new AudioComponent from the registered factory
		NodeInfo info = nodeManager.getNodeInfo(uiNode.get());
		component.reset(info.convertNodeToAudioComponent(uiNode.get()));
		_nodeMap[uiNode->id] = component;
	}

	// Keep UI ↔ backend references in sync
	uiNode->audioComponentId = component->id;
	uiNode->audioComponent = component.get();

	// Rebuild input connections from UI links
	component->clearInputs();

	auto inputs = getNodeInputs(uiNode, nodeManager, linkManager);
	for (size_t pinIdx = 0; pinIdx < inputs.size(); pinIdx++)
	{
		if (pinIdx >= uiNode->inputs.size()) break;
		int inputId = uiNode->inputs[pinIdx].inputId;
		if (inputId < 0) continue;

		for (auto& childUiNode : inputs[pinIdx])
		{
			auto childComponent = compileNode(childUiNode, nodeManager, linkManager, activeNodes);
			if (childComponent)
				component->addInput(inputId, childComponent);
		}
	}

	return component;
}

std::vector<std::vector<std::shared_ptr<Node>>> Compiler::getNodeInputs(
	const std::shared_ptr<Node>& node,
	NodeManager& nodeManager,
	LinkManager& linkManager)
{
	std::vector<std::vector<std::shared_ptr<Node>>> inputs;

	for (Pin& pin : node->inputs)
	{
		std::list<LinkInfo> links = linkManager.findPinLinks(pin.id, 1);
		std::vector<std::shared_ptr<Node>> inputNodes;

		for (LinkInfo& link : links)
		{
			std::shared_ptr<Node>& inputNode = nodeManager.findNodeByPinId(link.OutputId);
			inputNodes.push_back(inputNode);
		}

		inputs.push_back(inputNodes);
	}

	return inputs;
}


MidiSourceComponent* Compiler::findMidiSource(const std::shared_ptr<AudioComponent>& node)
{
	if (!node) return nullptr;
	if (auto* ms = dynamic_cast<MidiSourceComponent*>(node.get())) return ms;
	for (auto& input : node->inputs)
		for (auto& child : input)
			if (auto* ms = findMidiSource(child)) return ms;
	return nullptr;
}

bool Compiler::containsMidiSource(const std::shared_ptr<AudioComponent>& node)
{
	return findMidiSource(node) != nullptr;
}

void Compiler::debugPrintTree(const std::shared_ptr<AudioComponent>& node, int depth)
{
	if (!node) return;
	std::string indent(depth * 2, ' ');
	Logger::log("Compiler", Debug) << indent << node->componentName << " (id=" << node->id << ")" << std::endl;
	for (size_t i = 0; i < node->inputs.size(); i++)
		for (auto& child : node->inputs[i])
			debugPrintTree(child, depth + 1);
}
