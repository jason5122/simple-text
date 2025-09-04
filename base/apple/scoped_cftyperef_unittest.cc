#include "base/apple/scoped_cftyperef.h"
#include <CoreFoundation/CoreFoundation.h>
#include <gtest/gtest.h>

// This is effectively a unit test of ScopedTypeRef rather than ScopedCFTypeRef,
// but because ScopedTypeRef is parameterized, the CFType version is a great
// test subject because it uses all the features.
//
// Note that CFMutableArray is used for testing, even when subtypes aren't
// needed, because it is never optimized into immortal constant values, unlike
// other types.

namespace base::apple {

TEST(ScopedCFTypeRefTest, ConstructionSameType) {
    CFMutableArrayRef array =
        CFArrayCreateMutable(nullptr, /*capacity=*/0, &kCFTypeArrayCallBacks);
    EXPECT_EQ(1, CFGetRetainCount(array));

    ScopedCFTypeRef<CFMutableArrayRef> retain_scoper(array, OwnershipPolicy::RETAIN);
    EXPECT_EQ(array, retain_scoper.get());
    EXPECT_EQ(2, CFGetRetainCount(array));

    ScopedCFTypeRef<CFMutableArrayRef> assume_scoper(array, OwnershipPolicy::ASSUME);
    EXPECT_EQ(array, assume_scoper.get());
    EXPECT_EQ(2, CFGetRetainCount(array));

    CFMutableArrayRef array2 =
        CFArrayCreateMutable(nullptr, /*capacity=*/0, &kCFTypeArrayCallBacks);
    EXPECT_EQ(1, CFGetRetainCount(array2));
    ScopedCFTypeRef<CFMutableArrayRef> assume_scoper2(array2 /* with implicit ASSUME */);
    EXPECT_EQ(array2, assume_scoper2.get());
    EXPECT_EQ(1, CFGetRetainCount(array2));
}

TEST(ScopedCFTypeRefTest, ConstructionSubType) {
    CFMutableArrayRef array =
        CFArrayCreateMutable(nullptr, /*capacity=*/0, &kCFTypeArrayCallBacks);
    EXPECT_EQ(1, CFGetRetainCount(array));

    ScopedCFTypeRef<CFArrayRef> scoper(array);
    EXPECT_EQ(array, scoper.get());
    EXPECT_EQ(1, CFGetRetainCount(array));
}

// TEST(ScopedCFTypeRefTest, CopyConstructionSameType) {
//     ScopedCFTypeRef<CFMutableArrayRef> original(
//         CFArrayCreateMutable(nullptr, /*capacity=*/0, &kCFTypeArrayCallBacks));
//     EXPECT_EQ(1, CFGetRetainCount(original.get()));

//     ScopedCFTypeRef<CFMutableArrayRef> copy(original);
//     EXPECT_EQ(original.get(), copy.get());
//     EXPECT_EQ(2, CFGetRetainCount(original.get()));
// }

// TEST(ScopedCFTypeRefTest, CopyConstructionSubType) {
//     ScopedCFTypeRef<CFMutableArrayRef> original(
//         CFArrayCreateMutable(nullptr, /*capacity=*/0, &kCFTypeArrayCallBacks));
//     EXPECT_EQ(1, CFGetRetainCount(original.get()));

//     ScopedCFTypeRef<CFArrayRef> copy(original);
//     EXPECT_EQ(original.get(), copy.get());
//     EXPECT_EQ(2, CFGetRetainCount(original.get()));
// }

TEST(ScopedCFTypeRefTest, CopyConstructionReturnSubType) {
    auto subtype_returner = []() -> ScopedCFTypeRef<CFArrayRef> {
        ScopedCFTypeRef<CFMutableArrayRef> original(
            CFArrayCreateMutable(nullptr, /*capacity=*/0, &kCFTypeArrayCallBacks));
        return original;
    };
    EXPECT_TRUE(subtype_returner());
}

// TEST(ScopedCFTypeRefTest, CopyAssignmentSameType) {
//     ScopedCFTypeRef<CFMutableArrayRef> original(
//         CFArrayCreateMutable(nullptr, /*capacity=*/0, &kCFTypeArrayCallBacks));
//     EXPECT_EQ(1, CFGetRetainCount(original.get()));

//     ScopedCFTypeRef<CFMutableArrayRef> new_object(
//         CFArrayCreateMutable(nullptr, /*capacity=*/0, &kCFTypeArrayCallBacks));
//     EXPECT_EQ(1, CFGetRetainCount(new_object.get()));

//     original = new_object;
//     EXPECT_EQ(original.get(), new_object.get());
//     EXPECT_EQ(2, CFGetRetainCount(original.get()));
// }

// TEST(ScopedCFTypeRefTest, CopyAssignmentSubType) {
//     ScopedCFTypeRef<CFArrayRef> original(
//         CFArrayCreateMutable(nullptr, /*capacity=*/0, &kCFTypeArrayCallBacks));
//     EXPECT_EQ(1, CFGetRetainCount(original.get()));

//     ScopedCFTypeRef<CFMutableArrayRef> new_object(
//         CFArrayCreateMutable(nullptr, /*capacity=*/0, &kCFTypeArrayCallBacks));
//     EXPECT_EQ(1, CFGetRetainCount(new_object.get()));

//     original = new_object;
//     EXPECT_EQ(original.get(), new_object.get());
//     EXPECT_EQ(2, CFGetRetainCount(original.get()));
// }

TEST(ScopedCFTypeRefTest, MoveConstructionSameType) {
    ScopedCFTypeRef<CFMutableArrayRef> original(
        CFArrayCreateMutable(nullptr, /*capacity=*/0, &kCFTypeArrayCallBacks));
    CFMutableArrayRef original_ref = original.get();
    EXPECT_EQ(1, CFGetRetainCount(original.get()));

    ScopedCFTypeRef<CFMutableArrayRef> copy(std::move(original));
    EXPECT_EQ(nullptr, original.get());
    EXPECT_EQ(original_ref, copy.get());
    EXPECT_EQ(1, CFGetRetainCount(copy.get()));
}

TEST(ScopedCFTypeRefTest, MoveConstructionSubType) {
    ScopedCFTypeRef<CFMutableArrayRef> original(
        CFArrayCreateMutable(nullptr, /*capacity=*/0, &kCFTypeArrayCallBacks));
    CFMutableArrayRef original_ref = original.get();
    EXPECT_EQ(1, CFGetRetainCount(original.get()));

    ScopedCFTypeRef<CFArrayRef> copy(std::move(original));
    EXPECT_EQ(nullptr, original.get());
    EXPECT_EQ(original_ref, copy.get());
    EXPECT_EQ(1, CFGetRetainCount(copy.get()));
}

namespace {
class MoveConstructionReturnTest {
public:
    MoveConstructionReturnTest()
        : array_(CFArrayCreateMutable(nullptr,
                                      /*capacity=*/0,
                                      &kCFTypeArrayCallBacks)) {}

    ScopedCFTypeRef<CFMutableArrayRef> take_array() { return std::move(array_); }

    bool has_array() { return array_.get() != nullptr; }

private:
    ScopedCFTypeRef<CFMutableArrayRef> array_;
};
}  // namespace

TEST(ScopedCFTypeRefTest, MoveConstructionReturn) {
    MoveConstructionReturnTest test;
    ASSERT_TRUE(test.has_array());
    ASSERT_TRUE(test.take_array());
    ASSERT_FALSE(test.has_array());
    ASSERT_FALSE(test.take_array());
}

TEST(ScopedCFTypeRefTest, MoveAssignmentSameType) {
    ScopedCFTypeRef<CFMutableArrayRef> original(
        CFArrayCreateMutable(nullptr, /*capacity=*/0, &kCFTypeArrayCallBacks));
    EXPECT_EQ(1, CFGetRetainCount(original.get()));

    ScopedCFTypeRef<CFMutableArrayRef> new_object(
        CFArrayCreateMutable(nullptr, /*capacity=*/0, &kCFTypeArrayCallBacks));
    CFMutableArrayRef new_ref = new_object.get();
    EXPECT_EQ(1, CFGetRetainCount(new_object.get()));

    original = std::move(new_object);
    EXPECT_EQ(nullptr, new_object.get());
    EXPECT_EQ(new_ref, original.get());
    EXPECT_EQ(1, CFGetRetainCount(original.get()));
}

TEST(ScopedCFTypeRefTest, MoveAssignmentSubType) {
    ScopedCFTypeRef<CFArrayRef> original(
        CFArrayCreateMutable(nullptr, /*capacity=*/0, &kCFTypeArrayCallBacks));
    EXPECT_EQ(1, CFGetRetainCount(original.get()));

    ScopedCFTypeRef<CFMutableArrayRef> new_object(
        CFArrayCreateMutable(nullptr, /*capacity=*/0, &kCFTypeArrayCallBacks));
    CFMutableArrayRef new_ref = new_object.get();
    EXPECT_EQ(1, CFGetRetainCount(new_object.get()));

    original = std::move(new_object);
    EXPECT_EQ(nullptr, new_object.get());
    EXPECT_EQ(new_ref, original.get());
    EXPECT_EQ(1, CFGetRetainCount(original.get()));
}

// TEST(ScopedCFTypeRefTest, ResetSameType) {
//     CFMutableArrayRef array =
//         CFArrayCreateMutable(nullptr, /*capacity=*/0, &kCFTypeArrayCallBacks);
//     EXPECT_EQ(1, CFGetRetainCount(array));

//     ScopedCFTypeRef<CFMutableArrayRef> retain_scoper;
//     retain_scoper.reset(array, OwnershipPolicy::RETAIN);
//     EXPECT_EQ(array, retain_scoper.get());
//     EXPECT_EQ(2, CFGetRetainCount(array));

//     ScopedCFTypeRef<CFMutableArrayRef> assume_scoper;
//     assume_scoper.reset(array, OwnershipPolicy::ASSUME);
//     EXPECT_EQ(array, assume_scoper.get());
//     EXPECT_EQ(2, CFGetRetainCount(array));

//     CFMutableArrayRef array2 =
//         CFArrayCreateMutable(nullptr, /*capacity=*/0, &kCFTypeArrayCallBacks);
//     EXPECT_EQ(1, CFGetRetainCount(array2));
//     ScopedCFTypeRef<CFMutableArrayRef> assume_scoper2;
//     assume_scoper2.reset(array2 /* with implicit ASSUME */);
//     EXPECT_EQ(array2, assume_scoper2.get());
//     EXPECT_EQ(1, CFGetRetainCount(array2));
// }

// TEST(ScopedCFTypeRefTest, ResetSubType) {
//     CFMutableArrayRef array =
//         CFArrayCreateMutable(nullptr, /*capacity=*/0, &kCFTypeArrayCallBacks);
//     EXPECT_EQ(1, CFGetRetainCount(array));

//     ScopedCFTypeRef<CFArrayRef> scoper;
//     scoper.reset(array);
//     EXPECT_EQ(array, scoper.get());
//     EXPECT_EQ(1, CFGetRetainCount(array));
// }

// TEST(ScopedCFTypeRefTest, ResetFromScoperSameType) {
//     ScopedCFTypeRef<CFMutableArrayRef> original(
//         CFArrayCreateMutable(nullptr, /*capacity=*/0, &kCFTypeArrayCallBacks));
//     EXPECT_EQ(1, CFGetRetainCount(original.get()));

//     ScopedCFTypeRef<CFMutableArrayRef> copy;
//     copy.reset(original);
//     EXPECT_EQ(original.get(), copy.get());
//     EXPECT_EQ(2, CFGetRetainCount(original.get()));
// }

// TEST(ScopedCFTypeRefTest, ResetFromScoperSubType) {
//     ScopedCFTypeRef<CFMutableArrayRef> original(
//         CFArrayCreateMutable(nullptr, /*capacity=*/0, &kCFTypeArrayCallBacks));
//     EXPECT_EQ(1, CFGetRetainCount(original.get()));

//     ScopedCFTypeRef<CFArrayRef> copy;
//     copy.reset(original);
//     EXPECT_EQ(original.get(), copy.get());
//     EXPECT_EQ(2, CFGetRetainCount(original.get()));
// }

}  // namespace base::apple
