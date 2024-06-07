#pragma once

#include <implot.h>
#include "inc.hpp"

#include "MidiMath.hpp"

class ImPlotUI {
private:
	float* _timeArray;

	// Envelope editor
	bool _printEnvelopeEditor = false;
	Vec2* _controlPoints = nullptr;

public:
	ImPlotUI(AudioData& audio);
	void update(AudioData& audio);
	void setPrintEnvelopeEditor(bool value, Vec2* controlPoints = nullptr);

private:
	void printEnvelopeEditor();
	void printEnvelopeEditorPoints();
	void handleControlPoints();
};
