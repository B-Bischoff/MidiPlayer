#include "AudioFileManager.hpp"

IDManager_<AudioFile> AudioFileManager::_audioFiles;

id AudioFileManager::addAudioFile(const fs::path& filepath)
{
	// Check if file is already opened
	const std::list<id> usedIds = _audioFiles.getUsedIds();
	for (const id& id : usedIds)
	{
		if (_audioFiles.get(id).path == filepath)
		{
			Logger::log("AudioFileManager", Debug) << "Already registered: " << filepath.string() << std::endl;
			return id;
		}
	}

	std::ifstream file(filepath, std::ios::binary | std::ios::ate);
	if (!file.is_open())
	{
		Logger::log("AudioFileManager", Warning) << "Could not open file: " << filepath.string() << std::endl;
		assert(0);
	}

	AudioFile audioFile;
	if (filepath.extension().string() == "mp3")
		audioFile = extractMP3AudioData(file, filepath.string());
	else
	{
		Logger::log("AudioFileManager", Warning) << "Only mp3 file are supported" << std::endl;
		assert(0);
	}

	file.close();

	audioFile.path = filepath;
	id id = _audioFiles.add(audioFile);
	return id;
}

AudioFile AudioFileManager::extractMP3AudioData(std::ifstream& file, const std::string& filename)
{
	// Read whole file content
	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);
	std::vector<uint8_t> mp3Data(size);
	if (!file.read(reinterpret_cast<char*>(mp3Data.data()), size))
	{
		Logger::log("AudioFileManager-MP3", Error) << "Failed to read file: " << filename << std::endl;
		assert(0);
	}

	// Decode MP3 meta-data
	mp3dec_ex_t mp3Decoder;
	if (mp3dec_ex_open_buf(&mp3Decoder, mp3Data.data(), mp3Data.size(), MP3D_SEEK_TO_SAMPLE) != 0)
	{
		Logger::log("AudioFileManager-MP3", Error) << "Failed to initialize MP3 decoder for file: " << filename << std::endl;
		assert(0);
	}

	// Read mp3 audio-data
	short* audioData = new short[mp3Decoder.samples];
	assert(audioData != nullptr && "AudioFileManager failed alloc");

	size_t samplesDecoded = mp3dec_ex_read(&mp3Decoder, audioData, mp3Decoder.samples);
	if (samplesDecoded == 0)
	{
		Logger::log("AudioFileManager-MP3", Error) << "Failed to decode MP3 data for file: " << filename << std::endl;
		mp3dec_ex_close(&mp3Decoder);
		assert(0);
	}

	AudioFile audioFile = {
		.path = fs::path(), // To be set in addAudioFile method
		.channels = (unsigned int)mp3Decoder.info.channels,
		.sampleRate = (unsigned int)mp3Decoder.info.hz,
		.dataLength = (unsigned int)mp3Decoder.samples,
		.data = audioData,
	};

	return audioFile;
}

const AudioFile& AudioFileManager::getAudioFile(const id& id)
{
	if (!_audioFiles.exists(id))
	{
		Logger::log("AudioFileManager", Error) << "GET, id this does exists: " << id << std::endl;
		assert(0);
	}
	return _audioFiles.get(id);
}
