#pragma once

#include <tsf.h>
#include "path.hpp"
#include "Logger.hpp"

class SoundFont {
public:
	~SoundFont();

	bool loadSoundFontFile(const fs::path& filepath, const unsigned int& sampleRate);
	tsf* getSoundFont();

private:
	tsf* _tinySoundFont = nullptr;

	void deleteLoadedSoundFont();
};
