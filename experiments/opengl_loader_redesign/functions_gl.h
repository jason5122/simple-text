#pragma once

#include "util/non_copyable.h"

#include <memory>
#include <string>

namespace opengl_redesign {

class FunctionsGL : util::NonCopyable {
public:
    FunctionsGL();
    ~FunctionsGL();

    void load_global_function_pointers();

private:
    void* load_proc_address(std::string_view function) const;

    class impl;
    std::unique_ptr<impl> pimpl;
};

}  // namespace opengl_redesign
