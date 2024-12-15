#pragma once

#include <iostream>
#include <set>
#include <assert.h>
#include "config.hpp"
#include "Logger.hpp"

#define INVALID_ID 0

class IDManager {
private:
	std::set<unsigned int> _ids;

public:
	IDManager(unsigned int maxID = DEFAULT_MAX_ID);

	unsigned int getID();
	unsigned int getID(unsigned int wantedId);
	void releaseID(unsigned int id);
};
