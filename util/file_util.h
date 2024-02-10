#pragma once

#include <filesystem>
#include <string>
namespace fs = std::filesystem;

std::string ReadFile(std::string file_name);

fs::path ResourcePath();
