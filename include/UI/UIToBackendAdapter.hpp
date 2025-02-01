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
	static void deleteComponentAndInputs(AudioComponent* component, AudioComponent* master);

private:
	static void removeComponentFromBackend(AudioComponent* component, AudioComponent* componentToRemove, unsigned int depth = 0);
	static void updateBackendNode(AudioComponent& master, AudioComponent& component, Node& node, NodeManager& nodeManager, LinkManager& linkManager);
	static AudioComponent* allocateAudioComponent(Node& node);

	// Print tree helpers
	static void drawTree(int depth, int inputIndex, std::vector<bool> drawVertical, const std::string& text);

	static void printUIDiff(AudioComponent* component, Node& node, LinkManager& linkManager, NodeManager& nodeManager, int depth = 0, int inputIndex = 1, std::vector<bool> drawVertical = {});
	static void printBackendDiff(AudioComponent* component, Node& node, LinkManager& linkManager, NodeManager& nodeManager, int depth = 0, int inputIndex = 1, std::vector<bool> drawVertical = {});

	static AudioComponent* getAudioComponent(AudioComponent* component, const unsigned int id);
	static bool idExists(AudioComponent* component, const unsigned int id);

	// Method actually find audio component id linked to node.
	// It does not check for the UI node internal id.
	static Node* getNode(Node& node, NodeManager& nodeManager, LinkManager& linkManager, const unsigned int id);
	static bool idExists(Node& node, NodeManager& nodeManager, LinkManager& linkManager, const unsigned int id);
};
