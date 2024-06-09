#include "functions_gl.h"

#include <libloaderapi.h.h>
#include <wingdi.h>

namespace opengl {

FunctionsGL::FunctionsGL() {}

FunctionsGL::~FunctionsGL() {}

void* FunctionsGL::loadProcAddress(const std::string& function) const {
    void* p = (void*)wglGetProcAddress(function.c_str());
    if (!p) {
        HMODULE module = LoadLibraryExA("opengl32.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
        p = (void*)GetProcAddress(module, function.c_str());
    }

    return p;
}

void FunctionsGL::initialize() {
    initProcsGL();
}

}
