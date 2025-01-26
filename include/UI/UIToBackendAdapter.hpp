#pragma once

#include <cstring>
#include "AudioBackend/Components/Components.hpp"
#include "UI/LinkManager.hpp"
#include "UI/NodeManager.hpp"

class UIToBackendAdapter {
public:
	static void updateBackend(Master& master, NodeManager& nodeManager, LinkManager& linkManager);

	static void printTree(AudioComponent* component, int depth = 0, int inputIndex = 1, std::vector<bool> drawVertical = {});
	static void printTree(Node& node, LinkManager& linkManager, NodeManager& nodeManager, int depth = 0, int inputIndex = 1, std::vector<bool> drawVertical = {});

private:
	static void deleteComponentAndInputs(AudioComponent* component);
	static void updateBackendNode(AudioComponent& component, Node& node, NodeManager& nodeManager, LinkManager& linkManager);
	static AudioComponent* allocateAudioComponent(Node& node);

	// Print tree helpers
	static void drawTree(int depth, int inputIndex, std::vector<bool> drawVertical, const std::string& text);
};
