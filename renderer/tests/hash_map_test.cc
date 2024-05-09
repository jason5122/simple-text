#include "absl/container/flat_hash_map.h"
#include "third_party/unordered_dense/ankerl/unordered_dense.h"
#include "util/profile_util.h"
#include "gtest/gtest.h"
#include <random>
#include <unordered_map>

static constexpr size_t kIterations = 10000000;

struct Dummy {
    int x;
    int y;
    int z;
};

TEST(HashMapTest, StringKey) {
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist6(1, kIterations);

    {
        PROFILE_BLOCK_WITH_DURATION("StringKey: STL", std::chrono::milliseconds);
        std::unordered_map<std::string, Dummy> cache_stl;
        for (size_t i = 0; i < kIterations; i++) {
            std::string key = std::to_string(dist6(rng));
            Dummy dummy{
                .x = 1,
                .y = 2,
                .z = 3,
            };
            cache_stl.insert({std::move(key), std::move(dummy)});
        }
    }
    // {
    //     PROFILE_BLOCK_WITH_DURATION("StringKey: Abseil", std::chrono::milliseconds);
    //     absl::flat_hash_map<std::string, Dummy> cache_absl;
    //     for (size_t i = 0; i < kIterations; i++) {
    //         std::string key = std::to_string(dist6(rng));
    //         Dummy dummy{
    //             .x = 1,
    //             .y = 2,
    //             .z = 3,
    //         };
    //         cache_absl.insert({std::move(key), std::move(dummy)});
    //     }
    // }
    {
        PROFILE_BLOCK_WITH_DURATION("StringKey: Ankerl", std::chrono::milliseconds);
        ankerl::unordered_dense::map<std::string, Dummy> cache_ankerl;
        for (size_t i = 0; i < kIterations; i++) {
            std::string key = std::to_string(dist6(rng));
            Dummy dummy{
                .x = 1,
                .y = 2,
                .z = 3,
            };
            cache_ankerl.insert({std::move(key), std::move(dummy)});
        }
    }
}

TEST(HashMapTest, IntegerKey) {
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist6(1, kIterations);

    {
        PROFILE_BLOCK_WITH_DURATION("IntegerKey: STL", std::chrono::milliseconds);
        std::unordered_map<uint32_t, Dummy> cache_stl;
        for (size_t i = 0; i < kIterations; i++) {
            uint32_t key = dist6(rng);
            Dummy dummy{
                .x = 1,
                .y = 2,
                .z = 3,
            };
            cache_stl.insert({std::move(key), std::move(dummy)});
        }
    }
    // {
    //     PROFILE_BLOCK_WITH_DURATION("IntegerKey: Abseil", std::chrono::milliseconds);
    //     absl::flat_hash_map<uint32_t, Dummy> cache_absl;
    //     for (size_t i = 0; i < kIterations; i++) {
    //         uint32_t key = dist6(rng);
    //         Dummy dummy{
    //             .x = 1,
    //             .y = 2,
    //             .z = 3,
    //         };
    //         cache_absl.insert({std::move(key), std::move(dummy)});
    //     }
    // }
    {
        PROFILE_BLOCK_WITH_DURATION("IntegerKey: Ankerl", std::chrono::milliseconds);
        ankerl::unordered_dense::map<uint32_t, Dummy> cache_ankerl;
        for (size_t i = 0; i < kIterations; i++) {
            uint32_t key = dist6(rng);
            Dummy dummy{
                .x = 1,
                .y = 2,
                .z = 3,
            };
            cache_ankerl.insert({std::move(key), std::move(dummy)});
        }
    }
}
