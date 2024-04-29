#pragma once

#include <filesystem>
#include <string>
namespace fs = std::filesystem;

std::string ReadFile(fs::path file_name);

fs::path ResourceDir();

fs::path DataDir();
