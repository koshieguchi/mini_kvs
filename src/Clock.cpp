
#include "clock.h"

Clock::Clock() { this->handle = 0; }

void Clock::Insert(Page *page) {
    auto it = std::next(this->pages.begin(), this->handle);
    this->pages.insert(it, page);
    page->SetAccessBit(0);
}

void Clock::UpdatePageAccessStatus(Page *accessed_page) { accessed_page->SetAccessBit(1); }

Page *Clock::GetPageToEvict() {
    auto it = std::next(this->pages.begin(), this->handle);
    Page *cur_page = *it;
    while (cur_page->GetAccessBit()) {
        cur_page->SetAccessBit(0);
        this->handle++;
        ++it;
        if (it == this->pages.end()) {
            it = this->pages.begin();
            this->handle = 0;
        }
        cur_page = *it;
    }

    // Delete the evicted page from pages
    it = std::next(this->pages.begin(), this->handle);
    this->pages.erase(it);

    // Make sure the handle refers to a valid index after pages has shrunk.
    if (this->handle >= this->pages.size()) {
        this->handle = 0;
    }
    return cur_page;
}
