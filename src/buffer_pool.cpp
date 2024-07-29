
#include "buffer_pool.h"

#include <cmath>
#include <utility>

#include "clock.h"
#include "lru.h"

BufferPool::BufferPool(int min_size, int max_size, EvictionPolicyType eviction_policy_type) {
    this->hashtable = new ExtendibleHashtable(min_size, max_size);
    if (eviction_policy_type == EvictionPolicyType::LRU_t) {
        this->policy = new LRU();
    } else {
        this->policy = new Clock();
    }
}

BufferPool::~BufferPool() {
    delete this->hashtable;
    delete this->policy;
}

std::vector<uint64_t> BufferPool::Get(const std::string &page_id) {
    std::vector<uint64_t> pageData;
    Page *accessed_page = this->hashtable->Get(page_id);
    if (accessed_page != nullptr) {
        this->policy->UpdatePageAccessStatus(accessed_page);
        return accessed_page->GetData();
    }
    return {};
}

void BufferPool::Resize(int new_max_size) {
    int num_to_evict = std::ceil(this->hashtable->GetSize() - (ExtendibleHashtable::EXPAND_THRESHOLD * new_max_size));
    if (num_to_evict > 0) {
        for (int i = 0; i < num_to_evict; i++) {
            this->Evict();
        }

        // Try to shrink the hash table
        this->hashtable->Shrink();
    }
    this->hashtable->SetMaxSize(new_max_size);
}

void BufferPool::Insert(const std::string &page_id, std::vector<uint64_t> &data) {
    // Expand the directory if the total number of pages mapped to this hash table
    // is greater than a certain directory size threshold.
    if (this->hashtable->GetSize() > this->hashtable->GetNumDirectory() * ExtendibleHashtable::EXPAND_THRESHOLD) {
        bool need_to_evict = !this->hashtable->ExpandDirectory();
        if (need_to_evict) {  // Evict when directory can't be expanded anymore
            this->Evict();
        }
    }

    Page *new_page = new Page(page_id, data);
    this->hashtable->Insert(new_page);
    this->policy->Insert(new_page);
}

void BufferPool::Evict() {
    Page *page_to_evict = this->policy->GetPageToEvict();
    this->hashtable->Remove(page_to_evict);
}
