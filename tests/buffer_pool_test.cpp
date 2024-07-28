#include "buffer_pool.hpp"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>

#include "utils/test_utils.hpp"

void test_get() {
    BufferPool buffer_pool(2, 5);  // min_size, max_size
    std::vector<uint32_t> data = {1, 100, 2, 200};
    buffer_pool.insert("page1", (char*)&data);

    auto result = buffer_pool.get("page1");

    assert(result == (char*)&data);
    std::cout << "test_get passed!\n";
}

void test_resize() {
    BufferPool buffer_pool(2, 5);
    std::vector<uint32_t> data1 = {1, 100};
    std::vector<uint32_t> data2 = {2, 200};
    std::vector<uint32_t> data3 = {3, 300};
    buffer_pool.insert("page1", (char*)&data1);
    buffer_pool.insert("page2", (char*)&data2);
    buffer_pool.insert("page3", (char*)&data3);

    buffer_pool.resize(2);  // Resize to a smaller size, should trigger eviction.

    assert(buffer_pool.get_all_pages().size() <= 2);
    std::cout << "test_resize passed!\n";
}

void test_insert() {
    BufferPool buffer_pool(2, 5);

    std::vector<uint32_t> data1 = {1, 100};
    std::vector<uint32_t> data2 = {2, 200};
    buffer_pool.insert("page1", (char*)&data1);
    buffer_pool.insert("page2", (char*)&data2);

    auto pages = buffer_pool.get_all_pages();

    assert(pages.size() == 2);
    assert(buffer_pool.get("page1") == (char*)&data1);
    assert(buffer_pool.get("page2") == (char*)&data2);
    std::cout << "test_insert passed!\n";
}

void test_get_all_pages() {
    BufferPool buffer_pool(2, 5);
    std::vector<uint32_t> data1 = {1, 100};
    std::vector<uint32_t> data2 = {2, 200};
    buffer_pool.insert("page1", (char*)&data1);
    buffer_pool.insert("page2", (char*)&data2);

    auto pages = buffer_pool.get_all_pages();

    assert(pages.size() == 2);
    std::cout << "test_get_all_pages passed!\n";
}

int main() {
    test_get();
    test_resize();
    test_insert();
    test_get_all_pages();

    std::cout << GREEN << "All tests passed!\n" << RESET << std::endl;

    return 0;
}
