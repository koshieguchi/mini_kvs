#include "bucket.hpp"

#include <iostream>

Bucket::Bucket(int depth) {
    this->local_depth = depth;
    this->size = 0;
}

Bucket::~Bucket() {
    for (Page *page : pages) {
        delete page;
    }
}

Page *Bucket::get_page(const std::string &page_id) {
    for (Page *page : this->pages) {
        if (page->get_page_id() == page_id) {
            return page;
        }
    }
    return nullptr;
}

void Bucket::insert_page(Page *new_page) {
    this->size++;
    this->pages.push_front(new_page);
}

void Bucket::remove_page(Page *page_to_remove) {
    this->pages.remove(page_to_remove);
    delete page_to_remove;
    this->size--;
}

int Bucket::get_size() const { return this->size; }

int Bucket::get_local_depth() const { return this->local_depth; }

void Bucket::increment_local_depth() { this->local_depth++; }

void Bucket::decrement_local_depth() { this->local_depth--; }

std::forward_list<Page *> Bucket::get_pages() { return this->pages; }

void Bucket::clear() {
    this->pages.clear();
    this->size = 0;
}
