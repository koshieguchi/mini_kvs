
#include "bucket.h"

Bucket::Bucket(int depth) {
    this->local_depth = depth;
    this->size = 0;
}

Bucket::~Bucket() {
    for (Page *page : pages) {
        delete page;
    }
}

Page *Bucket::Get(const std::string &page_id) {
    for (Page *page : this->pages) {
        if (page->GetPageId() == page_id) {
            return page;
        }
    }
    return nullptr;
}

void Bucket::Insert(Page *new_page) {
    this->size++;
    this->pages.push_front(new_page);
}

void Bucket::Remove(Page *page_to_remove) {
    this->pages.remove(page_to_remove);
    delete page_to_remove;
    this->size--;
}

int Bucket::GetSize() const { return this->size; }

int Bucket::GetLocalDepth() const { return this->local_depth; }

void Bucket::IncreaseLocalDepth() { this->local_depth++; }

void Bucket::DecreaseLocalDepth() { this->local_depth--; }

std::forward_list<Page *> Bucket::GetPages() { return this->pages; }

void Bucket::Clear() {
    this->pages.clear();
    this->size = 0;
}
