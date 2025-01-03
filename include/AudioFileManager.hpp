#pragma once

#include "inc.hpp"
#include <unordered_map>
#include <queue>
#include <list>
#include <fstream>
#include <assert.h>

#include "Logger.hpp"
#include "AudioFile.hpp"

// [TODO] Can this be used to improve UI node&link managers?
using id = unsigned int;

template<typename T>
class IDManager_ {
private:
	std::unordered_map<id, T> data;
	std::queue<id> freeIds;

	// Never use id = 0 which can be used to represent an invalid id
	id nextId = 1;

public:
	id add(const T& element)
	{
		id id;

		if (!freeIds.empty()) // Reuse available id
		{
			id = freeIds.front();
			freeIds.pop();
		}
		else // Create new id
			id = nextId++;

		data[id] = element;
		return id;
	}

	void remove(const id& id)
	{
		auto it = data.find(id);
		if (it == data.end())
		{
			Logger::log("IDManager", Error) << "Remove called with invalid id: " << id << std::endl;
			assert(0);
		}
		data.erase(id);
		freeIds.push(id);
	}

	T& get(const id& id)
	{
		auto it = data.find(id);
		if (it == data.end())
		{
			Logger::log("IDManager", Error) << "Get called with invalid id: " << id << std::endl;
			assert(0);
		}
		return it->second;
	}

	bool exists(const id& id) const
	{
		return data.find(id) != data.end();
	}

	std::list<id> getUsedIds() const
	{
		std::list<id> usedIds;
		for (const auto& elem : data)
			usedIds.push_back(elem.first);

		return usedIds;
	}
};

class AudioFileManager {
private:
	static IDManager_<AudioFile> _audioFiles;

	static AudioFile extractMP3AudioData(std::ifstream& file, const std::string& filename);

public:
	static const AudioFile& getAudioFile(const id& id) ;
	static void removeAudioFile(const id& id) {
		// Free audio data
	}
	static id addAudioFile(const fs::path& filepath);
};
