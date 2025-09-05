#pragma once

#include <Foundation/Foundation.h>
#include <gtest/gtest.h>

namespace testing {
namespace internal {

// This overloaded version allows comparison between ObjC objects that conform
// to the NSObject protocol. Used to implement {ASSERT|EXPECT}_NSEQ().
AssertionResult CmpHelperNSEQ(const char* expected_expression,
                              const char* actual_expression,
                              id<NSObject> expected,
                              id<NSObject> actual);

// This overloaded version allows comparison between ObjC objects that conform
// to the NSObject protocol. Used to implement {ASSERT|EXPECT}_NSNE().
AssertionResult CmpHelperNSNE(const char* expected_expression,
                              const char* actual_expression,
                              id<NSObject> expected,
                              id<NSObject> actual);

// This overloaded version allows comparison between NSRect objects using
// NSEqualRects. Used to implement {ASSERT|EXPECT}_NSEQ().
AssertionResult CmpHelperNSEQ(const char* expected_expression,
                              const char* actual_expression,
                              const NSRect& expected,
                              const NSRect& actual);

// This overloaded version allows comparison between NSRect objects using
// NSEqualRects. Used to implement {ASSERT|EXPECT}_NSNE().
AssertionResult CmpHelperNSNE(const char* expected_expression,
                              const char* actual_expression,
                              const NSRect& expected,
                              const NSRect& actual);

// This overloaded version allows comparison between NSPoint objects using
// NSEqualPoints. Used to implement {ASSERT|EXPECT}_NSEQ().
AssertionResult CmpHelperNSEQ(const char* expected_expression,
                              const char* actual_expression,
                              const NSPoint& expected,
                              const NSPoint& actual);

// This overloaded version allows comparison between NSPoint objects using
// NSEqualPoints. Used to implement {ASSERT|EXPECT}_NSNE().
AssertionResult CmpHelperNSNE(const char* expected_expression,
                              const char* actual_expression,
                              const NSPoint& expected,
                              const NSPoint& actual);

// This overloaded version allows comparison between NSRange objects using
// NSEqualRanges. Used to implement {ASSERT|EXPECT}_NSEQ().
AssertionResult CmpHelperNSEQ(const char* expected_expression,
                              const char* actual_expression,
                              const NSRange& expected,
                              const NSRange& actual);

// This overloaded version allows comparison between NSRange objects using
// NSEqualRanges. Used to implement {ASSERT|EXPECT}_NSNE().
AssertionResult CmpHelperNSNE(const char* expected_expression,
                              const char* actual_expression,
                              const NSRange& expected,
                              const NSRange& actual);

}  // namespace internal
}  // namespace testing

// Tests that [expected isEqual:actual].
#define EXPECT_NSEQ(expected, actual)                                                             \
    EXPECT_PRED_FORMAT2(::testing::internal::CmpHelperNSEQ, expected, actual)
#define EXPECT_NSNE(val1, val2) EXPECT_PRED_FORMAT2(::testing::internal::CmpHelperNSNE, val1, val2)

#define ASSERT_NSEQ(expected, actual)                                                             \
    ASSERT_PRED_FORMAT2(::testing::internal::CmpHelperNSEQ, expected, actual)
#define ASSERT_NSNE(val1, val2) ASSERT_PRED_FORMAT2(::testing::internal::CmpHelperNSNE, val1, val2)
