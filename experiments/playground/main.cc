#include "base/check.h"

int main() {
    CHECK_EQ(2, 2);
    // CHECK_EQ(9 + 10, 21);
    NOTREACHED();
}
