#include "lru.h"

LRU::LRU() {
    this->eviction_queue_head = nullptr;
    this->most_recent = nullptr;
}

void LRU::Insert(Page *page) {
    auto *new_node = new EvictionQueueNode(page, nullptr, nullptr);
    page->SetEvictionQueueNode(new_node);

    if (this->eviction_queue_head == nullptr) {
        this->eviction_queue_head = new_node;
        this->most_recent = new_node;
        return;
    }

    this->most_recent->SetNext(new_node);
    new_node->SetPrev(this->most_recent);
    this->most_recent = new_node;
}

// Update the newly accessed page to be the most recently used page.
void LRU::UpdatePageAccessStatus(Page *accessed_page) {
    EvictionQueueNode *accessed_node = accessed_page->GetEvictionQueueNode();

    // No need to do anything the page being accessed is already the most recent
    if (accessed_node == this->most_recent) {
        return;
    }

    // Link prev node to next node:  prev <-> accessed_page <-> next  =>  prev <-> next
    EvictionQueueNode *prev = accessed_node->GetPrev();
    EvictionQueueNode *next = accessed_node->GetNext();
    if (prev != nullptr) {
        prev->SetNext(next);
    }
    if (next != nullptr) {
        next->SetPrev(prev);
    }

    if (accessed_node == this->eviction_queue_head) {
        this->eviction_queue_head = next;
    }

    // Put this newly accessed page after the most_recent
    if (this->most_recent != nullptr) {
        this->most_recent->SetNext(accessed_node);
    }
    accessed_node->SetPrev(this->most_recent);
    accessed_node->SetNext(nullptr);
    this->most_recent = accessed_node;
}

// Evict the least recently used page
Page *LRU::GetPageToEvict() {
    EvictionQueueNode *target_eviction_node = this->eviction_queue_head;

    // Delete the EvictionQueueNode of this page, which is the first node of the queue.
    EvictionQueueNode *next = target_eviction_node->GetNext();
    if (next != nullptr) {
        next->SetPrev(nullptr);
    }
    this->eviction_queue_head = next;

    Page *page_to_evict = target_eviction_node->GetPage();
    page_to_evict->SetEvictionQueueNode(nullptr);
    return page_to_evict;
}

EvictionQueueNode *LRU::GetQueueHead() { return this->eviction_queue_head; }
