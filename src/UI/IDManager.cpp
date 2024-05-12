#include "UI/IDManager.hpp"

IDManager::IDManager(unsigned int maxID)
{
	// ids must start at 1
	for (unsigned int i = 1; i <= maxID; i++)
		_ids.insert(i);

#if VERBOSE
	std::cout << "[IDManager] init with maxID: " << maxID << std::endl;
#endif
}

unsigned int IDManager::getID()
{
	if (_ids.empty())
	{
		std::cerr << "[IDManager] no more id available." << std::endl;
		assert(0);
		return INVALID_ID;
	}

	unsigned int id = *_ids.begin();
	_ids.erase(id);

#if ID_MANAGER_VERBOSE
	std::cout << "[IDManager] new id " << id << std::endl;
#endif

	return id;
}

unsigned int IDManager::getID(unsigned int wantedId)
{
	if (_ids.find(wantedId) == _ids.end())
	{
		std::cerr << "[IDManager] requested id is not available." << std::endl;
		assert(0);
		return INVALID_ID;
	}

	_ids.erase(wantedId);

#if ID_MANAGER_VERBOSE
	std::cout << "[IDManager] new (wanted) id " << wantedId << std::endl;
#endif

	return wantedId;
}

void IDManager::releaseID(unsigned int id)
{
	if (_ids.find(id) != _ids.end())
	{
		std::cerr << "[IDManager] cannot release id " << id << " which was not used." << std::endl;
		assert(0);
		return;
	}

	_ids.insert(id);

#if ID_MANAGER_VERBOSE
	std::cout << "[IDManager] release id " << id << std::endl;
#endif
}
