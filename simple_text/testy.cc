#include "example_dll.h"
#include "example_lib.h"
#include "testy.h"
#include <iostream>

extern "C" {
#include "testy2.h"
}

int main() {
    std::cerr << max("hi", "bye") << '\n';
    std::cerr << mystery() << '\n';
    std::cerr << add(7, 10) << '\n';
    std::cerr << subtract(7, 10) << '\n';
}
