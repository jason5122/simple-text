#include "base/check.h"
#include "base/no_destructor.h"
#include <gtest/gtest.h>

namespace base {

struct NotreachedOnDestroy {
    ~NotreachedOnDestroy() { NOTREACHED(); }
};

TEST(NoDestructorTest, SkipsDestructors) {
    NoDestructor<NotreachedOnDestroy> destructor_should_not_run;
}

}  // namespace base
