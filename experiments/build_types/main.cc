#include "experiments/build_types/greeter_source_set.h"
#include "experiments/build_types/greeter_static.h"
#include <print>

int main() {
    std::println("build-type demo (this executable is the link tool's output)");
    std::println("  cc/cxx: {}", SourceSetGreeting());
    std::println("  alink:  {}", StaticGreeting());
    std::println("  solink/solink_module produced libgreeter_shared and libgreeter_module "
                 "alongside this binary; inspect them with nm.");
    return 0;
}
