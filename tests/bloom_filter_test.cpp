#include "bloom_filter.hpp"

#include <cassert>
#include <iostream>
#include <string>

#include "test_utils.hpp"
#include "xxhash.h"

void test_bloom_filter() {
    BloomFilter filter(15, 10);

    uint32_t key = 1;
    filter.insert(key);

    for (int i = 0; i < 10; i++) {
        int index = XXH64(&key, sizeof(uint32_t), i) % (15 * 10);
        assert(filter.bitmap[index] == 1);
    }

    // Ensure that the key is present in the filter
    assert(filter.get(key));

    std::cout << "test_bloom_filter passed!" << std::endl;
}

int main() {
    test_bloom_filter();

    std::cout << GREEN << "All tests passed!\n" << RESET << std::endl;

    return 0;
}
