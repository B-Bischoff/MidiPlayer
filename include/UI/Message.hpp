#pragma once

#include <queue>
#include <string>
#include "path.hpp"
#include <MidiMath.hpp>

typedef unsigned int MessageId;

#define UI_NULL 0x00
#define UI_SHOW_ADSR_EDITOR 0x01
#define UI_UPDATE_ADSR 0x02
#define UI_NODE_DELETED 0x03
#define UI_ADSR_MODIFIED 0x04
#define MESSAGE_COPY 0x05
#define MESSAGE_CUT 0x06
#define MESSAGE_PASTE 0x07
#define UI_CREATE_INSTRUMENT 0x08
#define UI_CLEAR_FOCUS 0x09
#define UI_SHOW_FILE_BROWSER 0x0a
#define SEND_NODE_FILEPATH 0x0b
#define AUDIO_SAMPLE_RATE_UPDATED 0x0c
#define AUDIO_CHANNELS_UPDATED 0x0d

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

struct FileBrowserOpenData {
	std::string title;
	std::vector<std::string> filter;
	unsigned int nodeId; // node who made the call to get some file
};

struct NodeFilepathData {
	fs::path filepath;
	unsigned int nodeId;
};
