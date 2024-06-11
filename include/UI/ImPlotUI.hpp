#pragma once

#include <implot.h>
#include "inc.hpp"

#include "Message.hpp"
#include "MidiMath.hpp"

class ImPlotUI {
private:
	float* _timeArray;

	// Envelope editor
	bool _printEnvelopeEditor = false;
	Vec2* _controlPoints = nullptr;
	unsigned int _nodeId = 0; // Used to detect controlPoints invalidation when a node is deleted

public:
	ImPlotUI(AudioData& audio);
	void update(AudioData& audio, std::queue<Message>& messages);
	void setPrintEnvelopeEditor(bool value, Vec2* controlPoints = nullptr, unsigned int nodeId = 0);
	unsigned int getNodeId() const;

private:
	void printEnvelopeEditor(std::queue<Message>& messages);
	void printEnvelopeEditorPoints();
	void handleControlPoints(std::queue<Message>& messages);
};
