#include <epoxy/gl.h>
#include <iostream>

int main() {
    std::cerr << "Hello world!\n";
    std::cerr << epoxy_gl_version() << '\n';
}
