#include "base/apple/foundation_util.h"
#include "testing/gtest_mac.h"
#include <gtest/gtest.h>

namespace base::apple {

TEST(FoundationUtilTest, ObjCCast) {
    @autoreleasepool {
        id test_array = @[];
        id test_array_mutable = [NSMutableArray array];
        id test_data = [NSData data];
        id test_data_mutable = [NSMutableData dataWithCapacity:10];
        id test_date = [NSDate date];
        id test_dict = @{@"meaning" : @42};
        id test_dict_mutable = [NSMutableDictionary dictionaryWithCapacity:10];
        id test_number = @42;
        id test_null = [NSNull null];
        id test_set = [NSSet setWithObject:@"string object"];
        id test_set_mutable = [NSMutableSet setWithCapacity:10];
        id test_str = [NSString string];
        id test_str_const = @"bonjour";
        id test_str_mutable = [NSMutableString stringWithCapacity:10];

        // Make sure the allocations of NS types are good.
        EXPECT_TRUE(test_array);
        EXPECT_TRUE(test_array_mutable);
        EXPECT_TRUE(test_data);
        EXPECT_TRUE(test_data_mutable);
        EXPECT_TRUE(test_date);
        EXPECT_TRUE(test_dict);
        EXPECT_TRUE(test_dict_mutable);
        EXPECT_TRUE(test_number);
        EXPECT_TRUE(test_null);
        EXPECT_TRUE(test_set);
        EXPECT_TRUE(test_set_mutable);
        EXPECT_TRUE(test_str);
        EXPECT_TRUE(test_str_const);
        EXPECT_TRUE(test_str_mutable);

        // Casting the id correctly provides the same pointer.
        EXPECT_EQ(test_array, ObjCCast<NSArray>(test_array));
        EXPECT_EQ(test_array_mutable, ObjCCast<NSArray>(test_array_mutable));
        EXPECT_EQ(test_data, ObjCCast<NSData>(test_data));
        EXPECT_EQ(test_data_mutable, ObjCCast<NSData>(test_data_mutable));
        EXPECT_EQ(test_date, ObjCCast<NSDate>(test_date));
        EXPECT_EQ(test_dict, ObjCCast<NSDictionary>(test_dict));
        EXPECT_EQ(test_dict_mutable, ObjCCast<NSDictionary>(test_dict_mutable));
        EXPECT_EQ(test_number, ObjCCast<NSNumber>(test_number));
        EXPECT_EQ(test_null, ObjCCast<NSNull>(test_null));
        EXPECT_EQ(test_set, ObjCCast<NSSet>(test_set));
        EXPECT_EQ(test_set_mutable, ObjCCast<NSSet>(test_set_mutable));
        EXPECT_EQ(test_str, ObjCCast<NSString>(test_str));
        EXPECT_EQ(test_str_const, ObjCCast<NSString>(test_str_const));
        EXPECT_EQ(test_str_mutable, ObjCCast<NSString>(test_str_mutable));

        // When given an incorrect ObjC cast, provide nil.
        EXPECT_FALSE(ObjCCast<NSString>(test_array));
        EXPECT_FALSE(ObjCCast<NSString>(test_array_mutable));
        EXPECT_FALSE(ObjCCast<NSString>(test_data));
        EXPECT_FALSE(ObjCCast<NSString>(test_data_mutable));
        EXPECT_FALSE(ObjCCast<NSSet>(test_date));
        EXPECT_FALSE(ObjCCast<NSSet>(test_dict));
        EXPECT_FALSE(ObjCCast<NSNumber>(test_dict_mutable));
        EXPECT_FALSE(ObjCCast<NSNull>(test_number));
        EXPECT_FALSE(ObjCCast<NSDictionary>(test_null));
        EXPECT_FALSE(ObjCCast<NSDictionary>(test_set));
        EXPECT_FALSE(ObjCCast<NSDate>(test_set_mutable));
        EXPECT_FALSE(ObjCCast<NSData>(test_str));
        EXPECT_FALSE(ObjCCast<NSData>(test_str_const));
        EXPECT_FALSE(ObjCCast<NSArray>(test_str_mutable));

        // Giving a nil provides a nil.
        EXPECT_FALSE(ObjCCast<NSArray>(nil));
        EXPECT_FALSE(ObjCCast<NSData>(nil));
        EXPECT_FALSE(ObjCCast<NSDate>(nil));
        EXPECT_FALSE(ObjCCast<NSDictionary>(nil));
        EXPECT_FALSE(ObjCCast<NSNull>(nil));
        EXPECT_FALSE(ObjCCast<NSNumber>(nil));
        EXPECT_FALSE(ObjCCast<NSSet>(nil));
        EXPECT_FALSE(ObjCCast<NSString>(nil));

        // ObjCCastStrict: correct cast results in correct pointer being returned.
        EXPECT_EQ(test_array, ObjCCastStrict<NSArray>(test_array));
        EXPECT_EQ(test_array_mutable, ObjCCastStrict<NSArray>(test_array_mutable));
        EXPECT_EQ(test_data, ObjCCastStrict<NSData>(test_data));
        EXPECT_EQ(test_data_mutable, ObjCCastStrict<NSData>(test_data_mutable));
        EXPECT_EQ(test_date, ObjCCastStrict<NSDate>(test_date));
        EXPECT_EQ(test_dict, ObjCCastStrict<NSDictionary>(test_dict));
        EXPECT_EQ(test_dict_mutable, ObjCCastStrict<NSDictionary>(test_dict_mutable));
        EXPECT_EQ(test_number, ObjCCastStrict<NSNumber>(test_number));
        EXPECT_EQ(test_null, ObjCCastStrict<NSNull>(test_null));
        EXPECT_EQ(test_set, ObjCCastStrict<NSSet>(test_set));
        EXPECT_EQ(test_set_mutable, ObjCCastStrict<NSSet>(test_set_mutable));
        EXPECT_EQ(test_str, ObjCCastStrict<NSString>(test_str));
        EXPECT_EQ(test_str_const, ObjCCastStrict<NSString>(test_str_const));
        EXPECT_EQ(test_str_mutable, ObjCCastStrict<NSString>(test_str_mutable));

        // ObjCCastStrict: Giving a nil provides a nil.
        EXPECT_FALSE(ObjCCastStrict<NSArray>(nil));
        EXPECT_FALSE(ObjCCastStrict<NSData>(nil));
        EXPECT_FALSE(ObjCCastStrict<NSDate>(nil));
        EXPECT_FALSE(ObjCCastStrict<NSDictionary>(nil));
        EXPECT_FALSE(ObjCCastStrict<NSNull>(nil));
        EXPECT_FALSE(ObjCCastStrict<NSNumber>(nil));
        EXPECT_FALSE(ObjCCastStrict<NSSet>(nil));
        EXPECT_FALSE(ObjCCastStrict<NSString>(nil));
    }
}

TEST(FoundationUtilTest, FilePathToNSString) {
    EXPECT_NSEQ(nil, FilePathToNSString(FilePath()));
    EXPECT_NSEQ(@"/a/b", FilePathToNSString(FilePath("/a/b")));
    EXPECT_NSEQ(@"a/b", FilePathToNSString(FilePath("a/b")));
}

TEST(FoundationUtilTest, NSStringToFilePath) {
    EXPECT_EQ(FilePath(), NSStringToFilePath(nil));
    EXPECT_EQ(FilePath(), NSStringToFilePath(@""));
    EXPECT_EQ(FilePath("/a/b"), NSStringToFilePath(@"/a/b"));
    EXPECT_EQ(FilePath("a/b"), NSStringToFilePath(@"a/b"));
}

TEST(FoundationUtilTest, FilePathToCFString) {
    EXPECT_EQ(ScopedCFTypeRef<CFStringRef>(), FilePathToCFString(FilePath()));
    EXPECT_TRUE(CFEqual(CFSTR("/a/b"), FilePathToCFString(FilePath("/a/b")).get()));
    EXPECT_TRUE(CFEqual(CFSTR("a/b"), FilePathToCFString(FilePath("a/b")).get()));
}

TEST(FoundationUtilTest, CFStringToFilePath) {
    EXPECT_EQ(FilePath(), CFStringToFilePath(nil));
    EXPECT_EQ(FilePath(), CFStringToFilePath(CFSTR("")));
    EXPECT_EQ(FilePath("/a/b"), CFStringToFilePath(CFSTR("/a/b")));
    EXPECT_EQ(FilePath("a/b"), CFStringToFilePath(CFSTR("a/b")));
}

}  // namespace base::apple
