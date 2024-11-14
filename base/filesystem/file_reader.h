#pragma once

#include <filesystem>
#include <string>
namespace fs = std::filesystem;

namespace base {

std::string ReadFile(std::string_view file_name);
void WriteFile(std::string_view file_name, std::string_view contents);
void* ReadFileBinary(std::string_view file_name, size_t& file_size);

fs::path ResourceDir();
fs::path DataDir();

}  // namespace base
