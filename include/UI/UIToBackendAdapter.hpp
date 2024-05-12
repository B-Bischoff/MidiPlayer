#pragma once

#include "AudioBackend/Components/Components.hpp"
#include "UI/LinkManager.hpp"
#include "UI/NodeManager.hpp"

class UIToBackendAdapter {
public:
	static void updateBackend(Master& master, NodeManager& nodeManager, LinkManager& linkManager);

private:
	static void deleteComponentAndInputs(AudioComponent* component);
	static void updateBackendNode(AudioComponent& component, Node& node, NodeManager& nodeManager, LinkManager& linkManager);
	static AudioComponent* allocateAudioComponent(Node& node);
};
