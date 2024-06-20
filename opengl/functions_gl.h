#pragma once

#include "util/non_copyable.h"
#include <memory>
#include <string>

namespace opengl {

class FunctionsGL : util::NonCopyable {
public:
    FunctionsGL();
    ~FunctionsGL();

    void initialize();

private:
    void* loadProcAddress(const std::string& function) const;

    class impl;
    std::unique_ptr<impl> pimpl;
};

}
