#pragma once

#include <filesystem>
#include <string>
namespace fs = std::filesystem;

namespace base {

std::string ReadFile(std::string_view file_name);
void WriteFile(std::string_view file_name, std::string_view contents);

fs::path ResourceDir();
fs::path DataDir();

}
