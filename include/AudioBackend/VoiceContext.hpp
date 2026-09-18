#pragma once

#include <unordered_map>
#include <memory>
#include <typeindex>
#include "inc.hpp"

// Base class for per-voice state stored in VoiceContext
struct NodeState {
	virtual ~NodeState() {}
};

// Holds per-voice state for all nodes within a voice.
// Each active voice in a Polyphony node owns one VoiceContext.
struct VoiceContext {
	MidiInfo noteInfo = {};
	bool releasing = false;
	unsigned int generation = 0; // Incremented on each voice assignment

	// Maps node ID → per-voice state for that node
	std::unordered_map<unsigned int, std::unique_ptr<NodeState>> nodeStates;

	template<typename T>
	T& getState(unsigned int nodeId) {
		auto it = nodeStates.find(nodeId);
		if (it == nodeStates.end()) {
			auto state = std::make_unique<T>();
			T& ref = *state;
			nodeStates[nodeId] = std::move(state);
			return ref;
		}
		return *static_cast<T*>(it->second.get());
	}

	void clear() {
		nodeStates.clear();
		noteInfo = {};
		releasing = false;
		generation = 0;
	}
};

// Thread-local active voice context.
// Set by Polyphony before evaluating each voice's sub-graph.
// When null, nodes are not inside a polyphonic context.
inline thread_local VoiceContext* activeVoiceContext = nullptr;
