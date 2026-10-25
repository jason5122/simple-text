#include "experiments/function/function.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <print>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

void say_hello() { std::println("  hello from a function pointer"); }

// Reports whether a capture sits inside the wrapper's own 24-byte buffer or out on the heap, by
// checking its address against the wrapper's extent.
template <class Wrapper>
const char* placement(const Wrapper& wrapper, const void* capture) {
    auto begin = reinterpret_cast<std::uintptr_t>(&wrapper);
    auto p = reinterpret_cast<std::uintptr_t>(capture);
    return p >= begin && p < begin + sizeof(Wrapper) ? "inline" : "heap";
}

template <std::size_t N>
struct Payload {
    unsigned char bytes[N] = {};
};

// Announces its destruction, to show when a call-once callable's captures go away. Move-only, so
// any lambda holding one is move-only too.
struct Noisy {
    std::string name;

    explicit Noisy(std::string n) : name(std::move(n)) {}
    Noisy(Noisy&& other) noexcept : name(std::exchange(other.name, {})) {}
    ~Noisy() {
        if (!name.empty()) std::println("  ~Noisy({})", name);
    }
};

// An immediate-callback API takes a function_ref: nothing is allocated or owned, and a temporary
// lambda at the call site is safe because the callee is done with it before returning. This is the
// shape of Sublime's read_file_impl progress callback and tree walkers.
int count_if(const std::vector<int>& values, base::function_ref<bool(int)> pred) {
    int n = 0;
    for (int v : values) {
        if (pred(v)) n++;
    }
    return n;
}

bool is_even(int v) { return v % 2 == 0; }

// A Sublime-style sink API: callbacks arrive as function<... &&>&& and are queued until fired,
// the shape of px_set_timeout and the dialog completions.
class Timeouts {
public:
    void set_timeout(base::function<void() &&>&& callback) {
        pending_.push_back(std::move(callback));
    }

    void fire_all() {
        auto batch = std::move(pending_);
        pending_.clear();
        for (auto& callback : batch) std::move(callback)();
    }

private:
    std::vector<base::function<void() &&>> pending_;
};

}  // namespace

void example1() {
    std::println("-- function<void()>: pointers, lambdas, empty state, moves");
    base::function<void()> f = &say_hello;
    f();
    f = [] { std::println("  hello from a lambda"); };
    f();
    base::function<void()> g = std::move(f);
    std::println("  after move: source {}, destination {}", f ? "set" : "empty",
                 g ? "set" : "empty");
}

void example2() {
    std::println("-- storage: 24 bytes fit the buffer, 32 go to the heap");
    Payload<24> small;
    base::function<const void*()> small_fn = [small] { return static_cast<const void*>(&small); };
    Payload<32> big;
    base::function<const void*()> big_fn = [big] { return static_cast<const void*>(&big); };
    std::println("  24-byte capture: {}", placement(small_fn, small_fn()));
    std::println("  32-byte capture: {}", placement(big_fn, big_fn()));
}

void example3() {
    std::println("-- move-only captures, which std::function rejects");
    auto owned = std::make_unique<int>(42);
    base::function<int()> read = [p = std::move(owned)] { return *p; };
    std::println("  unique_ptr capture reads {}", read());
}

void example4() {
    std::println("-- function<void(int) &&>: the call consumes the callable");
    base::function<void(int) &&> once = [n = Noisy("capture")](int x) {
        std::println("  called with {} while holding {}", x, n.name);
    };
    std::println("  before call: {}", once ? "set" : "empty");
    std::move(once)(7);
    std::println("  after call: {}", once ? "set" : "empty");
}

void example5() {
    std::println("-- sink parameters and deferred firing");
    Timeouts timeouts;
    for (int i = 1; i <= 3; i++) timeouts.set_timeout([i] { std::println("  timeout {}", i); });
    std::println("  queued three; firing");
    timeouts.fire_all();
}

void example6() {
    std::println("-- copyable_function<int()>: copies carry their own state");
    base::copyable_function<int()> counter = [n = 0]() mutable { return ++n; };
    counter();
    counter();
    base::copyable_function<int()> copy = counter;
    counter();
    std::println("  original next: {}, copy next: {}", counter(), copy());
    copy = counter;
    std::println("  after copy-assign, original next: {}, copy next: {}", counter(), copy());
}

void example7() {
    std::println("-- function_ref<bool(int)>: borrowed, two words, no ownership");
    static_assert(sizeof(base::function_ref<bool(int)>) == 16);
    static_assert(std::is_trivially_copyable_v<base::function_ref<bool(int)>>);
    std::vector<int> values = {1, 2, 3, 4, 5, 6};
    int limit = 3;
    std::println("  values = {}", values);
    std::println("  above {}: {} numbers", limit,
                 count_if(values, [limit](int v) { return v > limit; }));
    std::println("  even: {} numbers", count_if(values, is_even));
}

int main() {
    static_assert(sizeof(base::function<void()>) == 32);
    static_assert(sizeof(base::function<void() &&>) == 32);
    static_assert(sizeof(base::copyable_function<int()>) == 32);

    // example1();
    // example2();
    // example3();
    // example4();
    // example5();
    // example6();
    example7();

    // This fails to compile:
    // std::function<void()> fn = [p = std::make_unique<int>(42)] {};
}
