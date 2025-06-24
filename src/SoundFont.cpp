#include "SoundFont.hpp"

SoundFont::~SoundFont()
{
	deleteLoadedSoundFont();
}

bool SoundFont::loadSoundFontFile(const fs::path& filepath, const unsigned int& sampleRate)
{
	// Delete previous loaded instrument
	deleteLoadedSoundFont();
	_tinySoundFont = nullptr;

	_tinySoundFont = tsf_load_filename(filepath.string().c_str());
	if (_tinySoundFont == nullptr)
	{
		Logger::log("SoundFontPlayer", Error) << "Could not open file: " << filepath.string() << std::endl;
		return true;
	}

	tsf_set_output(_tinySoundFont, TSF_MONO, sampleRate, 0);

	return false;
}

void SoundFont::deleteLoadedSoundFont()
{
	if (_tinySoundFont != nullptr)
	{
		tsf_note_off_all(_tinySoundFont);
		tsf_close(_tinySoundFont);
	}
}

tsf* SoundFont::getSoundFont()
{
	return _tinySoundFont;
}
