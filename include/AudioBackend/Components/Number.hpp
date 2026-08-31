#pragma once

#include "AudioComponent.hpp"

struct Number : public AudioComponent {
	float number;

	Number() : AudioComponent() { componentName = "Number"; }

	std::shared_ptr<AudioComponent> clone() const override {
		auto c = std::make_shared<Number>();
		c->number = number;
		return c;
	}

	double process(const AudioInfos& audioInfos, std::vector<MidiInfo>& keyPressed, int currentKey = 0) override
	{
		return number;
	}
};
