#pragma once

#include <filesystem>
#include <string>

std::string ReadFile(std::string file_name);

std::filesystem::path ResourcePath();
