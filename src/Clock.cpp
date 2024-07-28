
#include "clock.h"

Clock::Clock() { this->handle = 0; }

void Clock::Insert(Page *page) {
    auto it = std::next(this->pages.begin(), this->handle);
    this->pages.insert(it, page);
    page->SetAccessBit(0);
}

void Clock::UpdatePageAccessStatus(Page *accessedPage) { accessedPage->SetAccessBit(1); }

Page *Clock::GetPageToEvict() {
    auto it = std::next(this->pages.begin(), this->handle);
    Page *curPage = *it;
    while (curPage->GetAccessBit()) {
        curPage->SetAccessBit(0);
        this->handle++;
        ++it;
        if (it == this->pages.end()) {
            it = this->pages.begin();
            this->handle = 0;
        }
        curPage = *it;
    }

    // Delete the evicted page from pages
    it = std::next(this->pages.begin(), this->handle);
    this->pages.erase(it);

    // Make sure the handle refers to a valid index after pages has shrunk.
    if (this->handle >= this->pages.size()) {
        this->handle = 0;
    }
    return curPage;
}
