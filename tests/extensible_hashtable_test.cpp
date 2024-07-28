#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>

#include "extendible_hashtable.hpp"
#include "utils/test_utils.hpp"

void test_initialization() {
    ExtendibleHashtable *hashtable = new ExtendibleHashtable(2, 4, 3);  // min_size, max_size, bucket_max_size

    assert(hashtable->get_num_buckets() == 2);
    assert(hashtable->get_global_depth() == 1);

    std::cout << "test_initialization passed!" << std::endl;
}

void test_get_page() {
    ExtendibleHashtable *hashtable = new ExtendibleHashtable(2, 4, 3);

    // Create a sample page
    Page *page = new Page("page1", "1");

    // Insert the sample page
    hashtable->insert_page(page);

    // Assert that get_page returns the correct page for a given ID
    assert(hashtable->get_page("page1") == page);

    // Assert that get_page returns nullptr for a non-existing page ID
    assert(hashtable->get_page("non_existing") == nullptr);

    std::cout << "test_get_page passed!" << std::endl;
}

void test_insert_page() {
    ExtendibleHashtable *hashtable = new ExtendibleHashtable(2, 4, 3);

    // Create a sample page
    Page *page = new Page("page2", "2");

    // Before insertion, assert the size of the hashtable
    int size_before_insert = hashtable->get_size();

    // Insert the page
    hashtable->insert_page(page);

    // Assert that the page can be retrieved
    assert(hashtable->get_page("page2") == page);

    // Assert that the size of the hashtable increases
    assert(hashtable->get_size() > size_before_insert);

    std::cout << "test_insert_page passed!" << std::endl;
}

void test_remove_page() {
    ExtendibleHashtable *hashtable = new ExtendibleHashtable(2, 4, 3);

    // Create and insert a sample page
    Page *page = new Page("page3", "3");
    hashtable->insert_page(page);

    // Assert the page is present before removal
    assert(hashtable->get_page("page3") == page);

    // Store the size of the hashtable before removal
    int size_before_remove = hashtable->get_size();

    // Remove the page
    hashtable->remove_page(page);

    // Assert that the page is no longer retrievable
    assert(hashtable->get_page("page3") == nullptr);

    // Assert that the size of the hashtable decreases
    assert(hashtable->get_size() < size_before_remove);

    std::cout << "test_remove_page passed!" << std::endl;
}

// void test_expand_directory();

// void test_shrink_directory();

void test_get_size() {
    ExtendibleHashtable *hashtable = new ExtendibleHashtable(2, 4, 3);

    // Initially, the hashtable should be empty
    assert(hashtable->get_size() == 0);

    // Add a page and check size increment
    Page *page = new Page("test_page1", "1");
    hashtable->insert_page(page);
    assert(hashtable->get_size() == 1);

    std::cout << "test_get_size passed!" << std::endl;
}

void test_get_global_depth() {
    ExtendibleHashtable *hashtable = new ExtendibleHashtable(2, 4, 3);

    // Check the initial global depth
    assert(hashtable->get_global_depth() == 1);
}

void test_get_num_directory() {
    ExtendibleHashtable *hashtable = new ExtendibleHashtable(2, 4, 3);

    // The number of directory entries should be 2^global_depth
    assert(hashtable->get_num_directory() == pow(2, hashtable->get_global_depth()));

    std::cout << "test_get_global_depth passed!" << std::endl;
}

void test_set_max_size() {
    // Create an ExtendibleHashtable with initial parameters
    ExtendibleHashtable hashtable(4, 16,
                                  3);  // min_size, max_size, bucket_max_size

    // Test 1: Increase the max size
    hashtable.set_max_size(32);
    assert(hashtable.get_global_depth() <= std::floor(std::log2(32)));  // Global depth should not exceed log2(32)

    // Test 2: Decrease the max size below current min depth
    hashtable.set_max_size(2);
    // Global depth should adjust to new max depth
    assert(hashtable.get_global_depth() == std::floor(std::log2(2)));
    // Number of directories should not exceed 2^global_depth
    assert(hashtable.get_num_directory() <= (size_t)(1 << hashtable.get_global_depth()));

    std::cout << "test_set_max_size passed!" << std::endl;
}

// void test_set_min_size()

void test_get_pair_bucket_id() {
    std::string bucket_id = "111";
    std::string pair_bucket_id = ExtendibleHashtable::get_pair_bucket_id(bucket_id);

    // Check if the pair bucket ID is correctly calculated
    assert(pair_bucket_id == "011");

    std::cout << "test_get_pair_bucket_id passed!" << std::endl;
}

void test_get_all_pages() {
    ExtendibleHashtable *hashtable = new ExtendibleHashtable(2, 4, 3);

    // Add some pages
    hashtable->insert_page(new Page("page1", "1"));
    hashtable->insert_page(new Page("page2", "2"));

    // Retrieve all pages and check their count
    auto pages = hashtable->get_all_pages();
    assert(pages.size() == 2);

    std::cout << "test_get_all_pages passed!" << std::endl;
}

void test_expand_directory() {
    int min_size = 2;
    int max_size = 8;
    int bucket_max_size = 4;
    ExtendibleHashtable hashtable(min_size, max_size, bucket_max_size);

    // Check initial global depth
    int initial_depth = hashtable.get_global_depth();

    // Attempt to expand the directory
    bool expanded = hashtable.expand_directory();

    // Check if the global depth increased after expansion
    int new_depth = hashtable.get_global_depth();

    // Test 1: Check if expand_directory() returns true and increases depth
    assert(expanded && new_depth == initial_depth + 1);

    // Keep expanding until it reaches max depth
    while (hashtable.expand_directory()) {
    }

    // Test 2: Check if it respects max depth
    assert(hashtable.get_global_depth() == std::floor(std::log2(max_size)));

    std::cout << "test_expand_directory passed!" << std::endl;
}

void test_shrink_directory() {
    int min_size = 2;
    int max_size = 8;
    int bucket_max_size = 4;
    ExtendibleHashtable hashtable(min_size, max_size, bucket_max_size);

    // Expand the hashtable to its maximum size first to ensure we have room to
    // shrink
    while (hashtable.expand_directory()) {
    }

    // Check global depth after expansion
    int expanded_depth = hashtable.get_global_depth();

    // Perform shrink operation
    hashtable.shrink_directory();

    // Check the global depth after shrink operation
    int shrunk_depth = hashtable.get_global_depth();

    // Test 1: Check if shrink_directory() decreases the depth
    assert(shrunk_depth < expanded_depth);

    // Keep shrinking until it reaches min depth
    while (hashtable.get_global_depth() > std::floor(std::log2(min_size))) {
        hashtable.shrink_directory();
    }

    // Test 2: Check if it respects min depth
    assert(hashtable.get_global_depth() == std::floor(std::log2(min_size)));

    std::cout << "test_shrink_directory passed!" << std::endl;
}

int main() {
    test_initialization();
    test_get_page();
    test_insert_page();
    test_remove_page();
    test_get_size();
    test_get_global_depth();
    test_get_num_directory();
    test_set_max_size();
    // test_set_min_size();
    test_get_pair_bucket_id();
    test_get_all_pages();
    test_expand_directory();
    test_shrink_directory();

    std::cout << GREEN << "All tests passed!\n" << RESET << std::endl;

    return 0;
}
