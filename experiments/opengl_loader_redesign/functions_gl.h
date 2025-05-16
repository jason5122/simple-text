#pragma once

#include <string>

namespace opengl_redesign {

void load_global_function_pointers();

namespace internal {
void* load_proc_address(const char* fp);
}

}  // namespace opengl_redesign
