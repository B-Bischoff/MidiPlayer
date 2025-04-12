#pragma once

#include <queue>
#include <MidiMath.hpp>

// Use an enumeration would make more sense?
typedef unsigned int MessageId;
#define UI_NULL 0x00
#define UI_SHOW_ADSR_EDITOR 0x01
#define UI_UPDATE_ADSR 0x02
#define UI_NODE_DELETED 0x03
#define UI_ADSR_MODIFIED 0x04
#define MESSAGE_COPY 0x05
#define MESSAGE_CUT 0x06
#define MESSAGE_PASTE 0x07
#define UI_SHOW_FILE_BROWSER 0x08
#define LOAD_AUDIO_FILE 0x09
#define AUDIO_FILE_LOADED 0x0a
#define CANCEL_LOAD_AUDIO_FILE 0x0b

struct Message {
	MessageId id;
	void* data;

	Message(MessageId id = UI_NULL, void* data = nullptr)
		: id(id), data(data) {}
};

struct MessageNodeIdAndControlPoints {
	Vec2* controlPoints;
	unsigned int nodeId;
};
