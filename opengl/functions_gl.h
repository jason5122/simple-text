#pragma once

#include "opengl/dispatch_table_gl.h"

namespace opengl {

class FunctionsGL : public DispatchTableGL {
public:
    FunctionsGL();
    ~FunctionsGL() override;

    void initialize();

private:
    void* loadProcAddress(const std::string& function) const override;

    class impl;
    std::unique_ptr<impl> pimpl;
};

}
