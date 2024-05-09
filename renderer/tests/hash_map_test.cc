#include "absl/container/flat_hash_map.h"
#include "gtest/gtest.h"
#include <random>
#include <unordered_map>

static constexpr size_t kIterations = 10000000;

struct Dummy {
    int x;
    int y;
    int z;
};

TEST(HashMapTest, STL) {
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist6(1, kIterations);

    std::unordered_map<std::string, Dummy> cache;
    for (size_t i = 0; i < kIterations; i++) {
        std::string key = std::to_string(dist6(rng));
        Dummy dummy{
            .x = 1,
            .y = 2,
            .z = 3,
        };
        cache.insert({std::move(key), std::move(dummy)});
    }
}

TEST(HashMapTest, Abseil) {
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist6(1, kIterations);

    absl::flat_hash_map<std::string, Dummy> cache;
    for (size_t i = 0; i < kIterations; i++) {
        std::string key = std::to_string(dist6(rng));
        Dummy dummy{
            .x = 1,
            .y = 2,
            .z = 3,
        };
        cache.insert({std::move(key), std::move(dummy)});
    }
}
