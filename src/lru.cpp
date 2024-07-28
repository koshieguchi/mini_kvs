#include "lru.h"

LRU::LRU() {
    this->evictionQueueHead = nullptr;
    this->mostRecent = nullptr;
}

void LRU::Insert(Page *page) {
    auto *newNode = new EvictionQueueNode(page, nullptr, nullptr);
    page->SetEvictionQueueNode(newNode);

    if (this->evictionQueueHead == nullptr) {
        this->evictionQueueHead = newNode;
        this->mostRecent = newNode;
        return;
    }

    this->mostRecent->SetNext(newNode);
    newNode->SetPrev(this->mostRecent);
    this->mostRecent = newNode;
}

// Update the newly accessed page to be the most recently used page.
void LRU::UpdatePageAccessStatus(Page *accessedPage) {
    EvictionQueueNode *accessedNode = accessedPage->GetEvictionQueueNode();

    // No need to do anything the page being accessed is already the most recent
    if (accessedNode == this->mostRecent) {
        return;
    }

    // Link prev node to next node:  prev <-> accessedPage <-> next  =>  prev <-> next
    EvictionQueueNode *prev = accessedNode->GetPrev();
    EvictionQueueNode *next = accessedNode->GetNext();
    if (prev != nullptr) {
        prev->SetNext(next);
    }
    if (next != nullptr) {
        next->SetPrev(prev);
    }

    if (accessedNode == this->evictionQueueHead) {
        this->evictionQueueHead = next;
    }

    // Put this newly accessed page after the mostRecent
    if (this->mostRecent != nullptr) {
        this->mostRecent->SetNext(accessedNode);
    }
    accessedNode->SetPrev(this->mostRecent);
    accessedNode->SetNext(nullptr);
    this->mostRecent = accessedNode;
}

// Evict the least recently used page
Page *LRU::GetPageToEvict() {
    EvictionQueueNode *targetEvictionNode = this->evictionQueueHead;

    // Delete the EvictionQueueNode of this page, which is the first node of the queue.
    EvictionQueueNode *next = targetEvictionNode->GetNext();
    if (next != nullptr) {
        next->SetPrev(nullptr);
    }
    this->evictionQueueHead = next;

    Page *pageToEvict = targetEvictionNode->GetPage();
    pageToEvict->SetEvictionQueueNode(nullptr);
    return pageToEvict;
}

EvictionQueueNode *LRU::GetQueueHead() { return this->evictionQueueHead; }
