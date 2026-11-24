#pragma once

#include <imgui.h>
#include <string>

#include "AudioBackend/Instrument.hpp"
#include "AudioBackend/Components/Components.hpp"

// Debug window showing the live state of every Polyphony voice of an instrument
class VoicesUI {
public:
	void draw(const char* title, Instrument* instrument, bool* open);

private:
	void drawPolyphony(const Polyphony& polyphony, int index);
	void drawVoiceRow(const Polyphony::Voice& voice, int index);

	static std::string noteName(int midiNote);
	static std::string releaseNodeState(const AudioComponent* node);
};
