
#include "bucket.h"

Bucket::Bucket(int depth) {
    this->localDepth = depth;
    this->size = 0;
}

Bucket::~Bucket() {
    for (Page *page : pages) {
        delete page;
    }
}

Page *Bucket::Get(const std::string &pageId) {
    for (Page *page : this->pages) {
        if (page->GetPageId() == pageId) {
            return page;
        }
    }
    return nullptr;
}

void Bucket::Insert(Page *newPage) {
    this->size++;
    this->pages.push_front(newPage);
}

void Bucket::Remove(Page *pageToRemove) {
    this->pages.remove(pageToRemove);
    delete pageToRemove;
    this->size--;
}

int Bucket::GetSize() const { return this->size; }

int Bucket::GetLocalDepth() const { return this->localDepth; }

void Bucket::IncreaseLocalDepth() { this->localDepth++; }

void Bucket::DecreaseLocalDepth() { this->localDepth--; }

std::forward_list<Page *> Bucket::GetPages() { return this->pages; }

void Bucket::Clear() {
    this->pages.clear();
    this->size = 0;
}
