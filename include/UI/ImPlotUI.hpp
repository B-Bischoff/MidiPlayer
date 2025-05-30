#pragma once

#include <implot.h>
#include "inc.hpp"

#include "Audio.hpp"
#include "UI/Colors.hpp"
#include "Message.hpp"
#include "MidiMath.hpp"

class ImPlotUI {
private:
	ImPlotContext* _context;

	// Envelope editor
	bool _printEnvelopeEditor = false;
	Vec2* _controlPoints = nullptr;
	unsigned int _nodeId = 0; // Used to detect controlPoints invalidation when a node is deleted

	std::unique_ptr<ImVec4[]> _colormapColors; // Implot colormap colors
	ImPlotColormap _colormap;

public:
	ImPlotUI(Audio& audio);
	~ImPlotUI();

	void initStyle();

	void update(Audio& audio, std::queue<Message>& messages, MidiPlayerSettings& settings);
	void setPrintEnvelopeEditor(bool value, Vec2* controlPoints = nullptr, unsigned int nodeId = 0);
	unsigned int getNodeId() const;

private:
	void printEnvelopeEditor(std::queue<Message>& messages);
	void printEnvelopeEditorPoints();
	void handleControlPoints(std::queue<Message>& messages);

	void printPlot(Audio& audio, unsigned int offset = 0, bool stereo = false);
};
