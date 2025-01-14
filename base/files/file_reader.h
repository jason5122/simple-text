#pragma once

#include <memory>
#include <string>

namespace base {

std::string ReadFile(std::string_view file_name);
void WriteFile(std::string_view file_name, std::string_view contents);
std::unique_ptr<char[]> ReadFileBinary(std::string_view file_name, size_t& file_size);

std::string ResourceDir();
std::string DataDir();

}  // namespace base
