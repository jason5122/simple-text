#include <exception>
#include <print>
#include <stdexcept>
#include <string_view>

namespace {

struct Guard {
    ~Guard() { std::println("  ~Guard ran (stack unwound)"); }
};

[[noreturn]] void deeper() {
    Guard g;
    throw std::runtime_error("boom");
}

}  // namespace

int main(int argc, char** argv) {
    if (argc == 2 && std::string_view(argv[1]) == "caught") {
        try {
            deeper();
        } catch (const std::exception& e) {
            std::println("  caught std::exception&: {}", e.what());
        }
        std::println("exceptions OK: unwind + destructor + catch-by-base + what()");
        return 0;
    }

    std::println("throwing an uncaught exception (libc++abi will terminate)...");
    throw std::runtime_error("boom");
}
