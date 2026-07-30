#include <print>
#include <string_view>

#if defined(__has_feature)
#if __has_feature(address_sanitizer)
#define ADDRESS_SANITIZER 1
#endif
#endif
#if defined(__SANITIZE_ADDRESS__)
#define ADDRESS_SANITIZER 1
#endif

namespace {

// Reading a volatile keeps these values opaque to the optimizer; kept 0 so the
// arithmetic is a runtime no-op that the compiler can't fold.
volatile int kZero = 0;

// Sink so otherwise-dead loads/stores aren't optimized out.
volatile int g_sink = 0;

// --- ASan: memory-safety bugs ----------------------------------------------

void heap_buffer_overflow() {
    int* a = new int[4];
    a[4 + kZero] = 1;  // one past the end
    g_sink = a[0];
    delete[] a;
}

void heap_use_after_free() {
    int* a = new int[4];
    a[0] = 7;
    delete[] a;
    g_sink = a[0 + kZero];  // read freed memory
}

void stack_buffer_overflow() {
    int a[4] = {};
    g_sink = a[4 + kZero];  // one past the end
}

void double_free() {
    int* a = new int[4];
    delete[] a;
    delete[] (a + kZero);  // free the same block twice
}

int* volatile g_escaped = nullptr;

// Passing the local through a function boundary lets its address escape without
// tripping -Wreturn-stack-address.
[[gnu::noinline]] void stash(int* p) { g_escaped = p; }

void capture_local() {
    int local[4] = {};
    local[0] = 42;
    stash(local);
}

void stack_use_after_return() {
    capture_local();
    g_sink = g_escaped[0 + kZero];  // read a returned stack slot
}

// --- UBSan: undefined behavior ----------------------------------------------

void signed_integer_overflow() {
    int max = 2147483647 + kZero;  // INT_MAX, opaque
    g_sink = max + 1;
}

void integer_divide_by_zero() { g_sink = 100 / kZero; }

int* volatile g_null = nullptr;

void null_dereference() { g_sink = *g_null; }

// ---------------------------------------------------------------------------

struct Demo {
    std::string_view name;
    std::string_view sanitizer;
    void (*run)();
};

constexpr Demo kDemos[] = {
    {"heap-buffer-overflow", "ASan", heap_buffer_overflow},
    {"heap-use-after-free", "ASan", heap_use_after_free},
    {"stack-buffer-overflow", "ASan", stack_buffer_overflow},
    {"stack-use-after-return", "ASan", stack_use_after_return},
    {"double-free", "ASan", double_free},
    {"signed-integer-overflow", "UBSan", signed_integer_overflow},
    {"integer-divide-by-zero", "UBSan", integer_divide_by_zero},
    {"null-dereference", "UBSan", null_dereference},
};

void print_usage() {
    std::println("usage: sanitizer_demo <case>");
    std::println("cases:");
    for (const Demo& d : kDemos) {
        std::println("  {:<24} ({})", d.name, d.sanitizer);
    }
}

}  // namespace

int main(int argc, char** argv) {
#if !defined(ADDRESS_SANITIZER)
    std::println("warning: built without sanitizers; defects will not be reported");
    std::println("         (release or Windows build).");
#endif

    if (argc != 2) {
        print_usage();
        return 2;
    }

    std::string_view arg = argv[1];
    for (const Demo& d : kDemos) {
        if (d.name == arg) {
            std::println("Triggering {} bug: {}", d.sanitizer, d.name);
            d.run();
            // ASan aborts inside run(); recoverable UBSan checks fall through.
            std::println("returned - if a 'runtime error' printed above, {} caught it",
                         d.sanitizer);
            return 0;
        }
    }

    std::println("unknown case: {}", arg);
    print_usage();
    return 2;
}
