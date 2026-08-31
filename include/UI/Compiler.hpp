#pragma once

#include <memory>
#include <unordered_map>
#include <unordered_set>

#include "AudioBackend/Components/Components.hpp"
#include "AudioBackend/Components/MidiSourceComponent.hpp"
#include "AudioBackend/Components/Polyphony.hpp"
#include "AudioBackend/Instrument.hpp"

#include "UI/NodeManager.hpp"
#include "UI/LinkManager.hpp"

#include "Logger.hpp"

class Compiler {
public:
	void compile(Instrument& instrument, NodeManager& nodeManager, LinkManager& linkManager);

private:
	// Maps UI node ID → backend AudioComponent (persists across compile calls for diffing)
	std::unordered_map<unsigned int, std::shared_ptr<AudioComponent>> _nodeMap;

	// Maps sub-graph root UI node ID → Polyphony node (persists across compiles to preserve voice state)
	std::unordered_map<unsigned int, std::shared_ptr<Polyphony>> _polyMap;

	// Recursively compile a UI node and its inputs into AudioComponents
	std::shared_ptr<AudioComponent> compileNode(
		const std::shared_ptr<Node>& uiNode,
		NodeManager& nodeManager,
		LinkManager& linkManager,
		std::unordered_set<unsigned int>& activeNodes);

	// Get the UI nodes connected to each input pin of a node
	static std::vector<std::vector<std::shared_ptr<Node>>> getNodeInputs(
		const std::shared_ptr<Node>& node,
		NodeManager& nodeManager,
		LinkManager& linkManager);

	// Check if a sub-tree contains a MidiSourceComponent
	static bool containsMidiSource(const std::shared_ptr<AudioComponent>& node);

	// Find the first MidiSourceComponent in a sub-tree
	static MidiSourceComponent* findMidiSource(const std::shared_ptr<AudioComponent>& node);

	// Debug: log the compiled backend tree
	static void debugPrintTree(const std::shared_ptr<AudioComponent>& node, int depth = 0);
};
