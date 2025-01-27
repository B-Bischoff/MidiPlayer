#pragma once

#include <cstring>
#include "AudioBackend/Components/Components.hpp"
#include "UI/LinkManager.hpp"
#include "UI/NodeManager.hpp"
#include <list>

class UIToBackendAdapter {
public:
	static void updateBackend(Master& master, NodeManager& nodeManager, LinkManager& linkManager);

	static void printTreesDiff(AudioComponent* component, Node& node, LinkManager& linkManager, NodeManager& nodeManager);
	static void printTree(const AudioComponent* component, int depth = 0, int inputIndex = 1, std::vector<bool> drawVertical = {});
	static void printTree(const Node& node, LinkManager& linkManager, NodeManager& nodeManager, int depth = 0, int inputIndex = 1, std::vector<bool> drawVertical = {});

private:
	static void deleteComponentAndInputs(AudioComponent* component);
	static void updateBackendNode(AudioComponent& component, Node& node, NodeManager& nodeManager, LinkManager& linkManager);
	static AudioComponent* allocateAudioComponent(Node& node);

	// Print tree helpers
	static void drawTree(int depth, int inputIndex, std::vector<bool> drawVertical, const std::string& text);

	static void printUIDiff(const AudioComponent* component, Node& node, LinkManager& linkManager, NodeManager& nodeManager, int depth = 0, int inputIndex = 1, std::vector<bool> drawVertical = {});
	static void printBackendDiff(const AudioComponent* component, Node& node, LinkManager& linkManager, NodeManager& nodeManager, int depth = 0, int inputIndex = 1, std::vector<bool> drawVertical = {});

	static bool idExists(const AudioComponent* component, const unsigned int id);
	// Method actually find audio component id linked to node.
	// It does not check for the UI node internal id.
	static bool idExists(const Node& node, NodeManager& nodeManager, LinkManager& linkManager, const unsigned int id);
};
