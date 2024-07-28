#include "bucket.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>

#include "test_utils.hpp"

void test_get_page() {
    Bucket bucket(1);
    Page *page1 = new Page("page1", "1");
    Page *page2 = new Page("page2", "2");

    bucket.insert_page(page1);
    bucket.insert_page(page2);

    assert(bucket.get_page("page1") == page1);
    assert(bucket.get_page("page2") == page2);
    assert(bucket.get_page("non_existent") == nullptr);

    std::cout << "test_get_page passed!" << std::endl;
}

void test_insert_page() {
    Bucket bucket(1);
    Page *page = new Page("page1", "1");

    bucket.insert_page(page);
    assert(bucket.get_size() == 1);
    assert(bucket.get_page("page1") == page);

    std::cout << "test_insert_page passed!" << std::endl;
}

void test_remove_page() {
    Bucket bucket(1);
    Page *page = new Page("page1", "1");

    bucket.insert_page(page);
    assert(bucket.get_size() == 1);

    bucket.remove_page(page);
    assert(bucket.get_size() == 0);
    assert(bucket.get_page("page1") == nullptr);

    std::cout << "test_remove_page passed!" << std::endl;
}

void test_get_size() {
    Bucket bucket(1);
    assert(bucket.get_size() == 0);

    Page *page1 = new Page("page1", "1");
    bucket.insert_page(page1);
    assert(bucket.get_size() == 1);

    Page *page2 = new Page("page2", "2");
    bucket.insert_page(page2);
    assert(bucket.get_size() == 2);

    bucket.remove_page(page1);
    assert(bucket.get_size() == 1);

    std::cout << "test_get_size passed!" << std::endl;
}

void test_get_local_depth() {
    Bucket bucket(2);
    assert(bucket.get_local_depth() == 2);

    bucket.increment_local_depth();
    assert(bucket.get_local_depth() == 3);

    bucket.decrement_local_depth();
    assert(bucket.get_local_depth() == 2);

    std::cout << "test_get_local_depth passed!" << std::endl;
}

void test_get_pages() {
    Bucket bucket(1);

    Page *page1 = new Page("page1", "1");
    Page *page2 = new Page("page2", "2");

    bucket.insert_page(page1);
    bucket.insert_page(page2);

    auto pages = bucket.get_pages();
    assert(std::find(pages.begin(), pages.end(), page1) != pages.end());
    assert(std::find(pages.begin(), pages.end(), page2) != pages.end());

    std::cout << "test_get_pages passed!" << std::endl;
}

void test_increment_local_depth() {
    Bucket bucket(2);
    assert(bucket.get_local_depth() == 2);

    bucket.increment_local_depth();
    assert(bucket.get_local_depth() == 3);

    std::cout << "test_increment_local_depth passed!" << std::endl;
}

void test_decrement_local_depth() {
    Bucket bucket(3);
    assert(bucket.get_local_depth() == 3);

    bucket.decrement_local_depth();
    assert(bucket.get_local_depth() == 2);

    std::cout << "test_decrement_local_depth passed!" << std::endl;
}

int main() {
    test_get_page();
    test_insert_page();
    test_remove_page();
    test_get_size();
    test_get_local_depth();
    test_get_pages();
    test_increment_local_depth();
    test_decrement_local_depth();

    std::cout << GREEN << "All tests passed!\n" << RESET << std::endl;

    return 0;
}
