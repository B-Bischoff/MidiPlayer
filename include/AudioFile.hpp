#pragma once

#include "path.hpp"

struct AudioFile {
	fs::path path;

	unsigned int channels;
	unsigned int sampleRate;
	unsigned int dataLength;
	short* data;
};
