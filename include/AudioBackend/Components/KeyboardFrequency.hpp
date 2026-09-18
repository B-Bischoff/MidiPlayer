#pragma once

#include "MidiSourceComponent.hpp"
#include "AudioBackend/VoiceContext.hpp"

struct KeyboardFrequency : public MidiSourceComponent {
	static unsigned int keyIndex;

	std::vector<MidiInfo>* keyPressedRef = nullptr;

	KeyboardFrequency() : MidiSourceComponent() { componentName = "KeyboardFrequency"; }

	std::shared_ptr<AudioComponent> clone() const override {
		auto c = std::make_shared<KeyboardFrequency>();
		c->keyPressedRef = keyPressedRef;
		return c;
	}

	void setKeyPressedRef(std::vector<MidiInfo>* ref) { keyPressedRef = ref; }

	// Audio output: convert note to frequence
	double process(const AudioInfos& audioInfos, std::vector<MidiInfo>& keyPressed, int currentKey = 0) override
	{
		// Inside a polyphonic voice — use voice context note
		if (activeVoiceContext && activeVoiceContext->noteInfo.keyIndex > 0)
			return activeVoiceContext->noteInfo.keyIndex;

		assert(0 && "KeyboardFrequency should be used inside a Polyphony voice context or with a MIDI source upstream.");

		return 0.0;
	}

	// MIDI output: generate NoteOn/NoteOff events for Polyphony
	std::vector<MidiEvent> processMidi(const AudioInfos& audioInfos) override
	{
		std::vector<MidiEvent> events;
		const std::vector<MidiInfo>& keys = keyPressedRef ? *keyPressedRef : _emptyKeys;

		// NoteOn for newly pressed keys (not in previous frame)
		for (const MidiInfo& key : keys)
		{
			bool wasPressed = false;
			for (int prevNote : _previousNotes)
			{
				if (prevNote == key.keyIndex)
				{
					wasPressed = true;
					break;
				}
			}
			if (!wasPressed)
				events.push_back(MidiEvent::noteOn(key.keyIndex, key.velocity));
		}

		// NoteOff for keys released since last frame
		for (int prevNote : _previousNotes)
		{
			bool stillPressed = false;
			for (const MidiInfo& key : keys)
			{
				if (key.keyIndex == prevNote)
				{
					stillPressed = true;
					break;
				}
			}
			if (!stillPressed)
				events.push_back(MidiEvent::noteOff(prevNote));
		}

		// Update tracking
		_previousNotes.clear();
		for (const MidiInfo& key : keys)
			_previousNotes.push_back(key.keyIndex);

		return events;
	}

private:
	std::vector<int> _previousNotes;
	static inline const std::vector<MidiInfo> _emptyKeys = {};
};
