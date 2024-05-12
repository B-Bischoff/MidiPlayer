#pragma once

#include <implot.h>
#include "inc.hpp"

class ImPlotUI {
private:
	float* _timeArray;

public:
	ImPlotUI(AudioData& audio);
	void update(AudioData& audio);
};
