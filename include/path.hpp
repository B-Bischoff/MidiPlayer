#pragma once

#include <filesystem>

#define LOGO_DIR "logo"
#define INSTRUMENTS_DIR "instruments"
#define INSTRUMENTS_EXTENSION ".json"

namespace fs = std::filesystem;

struct ApplicationPath {
	fs::path application;
	fs::path ressourceDirectory;
};
