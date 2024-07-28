#include "lru.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>

#include "test_utils.hpp"

void test_linked_list() {
    LRU lru;

    lru.insert("1");

    assert(lru.front->key == "1");
    assert(lru.rear->key == "1");
    assert(lru.front == lru.rear);

    lru.insert("2");
    lru.insert("3");

    assert(lru.front->key == "3");
    assert(lru.front->next->key == "2");
    assert(lru.front->next->next->key == "1");
    assert(lru.rear->key == "1");
    assert(lru.rear->prev->key == "2");
    assert(lru.rear->prev->prev->key == "3");

    std::cout << "test_linked_list passed!" << std::endl;
}

void test_update() {
    LRU lru;

    lru.insert("1");
    lru.insert("2");
    assert(lru.front->key == "2");

    lru.update("1");
    assert(lru.front->key == "1");
    assert(lru.front->next->key == "2");
    assert(lru.rear->key == "2");
    assert(lru.rear->prev->key == "1");

    lru.update("1");
    assert(lru.front->key == "1");
    assert(lru.front->next->key == "2");
    assert(lru.rear->key == "2");
    assert(lru.rear->prev->key == "1");

    std::cout << "test_update passed!" << std::endl;
}

void test_evict() {
    LRU lru;

    lru.insert("1");
    assert(lru.evict() == "1");

    lru.insert("2");
    lru.insert("3");
    assert(lru.evict() == "2");

    lru.insert("4");
    lru.update("3");
    assert(lru.evict() == "4");

    std::cout << "test_evict passed!" << std::endl;
}

void test_remove() {
    LRU lru;

    lru.remove("1");

    lru.insert("1");
    lru.insert("2");
    lru.remove("1");
    assert(lru.front->key == "2");
    assert(lru.rear->key == "2");

    lru.insert("3");
    lru.insert("4");
    lru.remove("3");
    assert(lru.front->key == "4");
    assert(lru.front->next->key == "2");

    std::cout << "test_remove passed!" << std::endl;
}

int main() {
    test_linked_list();
    test_update();
    test_evict();
    test_remove();

    std::cout << GREEN << "All tests passed!\n" << RESET << std::endl;

    return 0;
}
