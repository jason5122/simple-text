#pragma once

namespace gl {

void load_global_function_pointers();

namespace internal {
void* load_proc_address(const char* fp);
}

}  // namespace gl
