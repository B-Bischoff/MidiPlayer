#pragma once

#include <queue>

typedef unsigned int MessageId;

#define UI_NULL 0x00
#define UI_SHOW_ADSR_EDITOR 0x01
#define UI_UPDATE_ADSR 0x02

struct Message {
	MessageId id;
	void* data;

	Message(MessageId id = UI_NULL, void* data = nullptr)
		: id(id), data(data) {}
};
