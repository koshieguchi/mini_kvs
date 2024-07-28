
#include <utility>
#include <cmath>
#include "BufferPool.h"
#include "LRU.h"
#include "Clock.h"

BufferPool::BufferPool(int minSize, int maxSize, EvictionPolicyType evictionPolicyType) {
    this->hashtable = new ExtendibleHashtable(minSize, maxSize);
    if (evictionPolicyType == EvictionPolicyType::LRU_t) {
        this->policy = new LRU();
    } else {
        this->policy = new Clock();
    }
}

BufferPool::~BufferPool() {
    delete this->hashtable;
    delete this->policy;
}

std::vector<uint64_t> BufferPool::Get(const std::string &pageId) {
    std::vector<uint64_t> pageData;
    Page *accessedPage = this->hashtable->Get(pageId);
    if (accessedPage != nullptr) {
        this->policy->UpdatePageAccessStatus(accessedPage);
        return accessedPage->GetData();
    }
    return {};
}

void BufferPool::Resize(int newMaxSize) {
    int numToEvict = std::ceil(this->hashtable->GetSize() - (ExtendibleHashtable::EXPAND_THRESHOLD * newMaxSize));
    if (numToEvict > 0) {
        for (int i = 0; i < numToEvict; i++) {
            this->Evict();
        }

        // Try to shrink the hash table
        this->hashtable->Shrink();
    }
    this->hashtable->SetMaxSize(newMaxSize);
}

void BufferPool::Insert(const std::string &pageId, std::vector<uint64_t> &data) {
    // Expand the directory if the total number of pages mapped to this hash table
    // is greater than a certain directory size threshold.
    if (this->hashtable->GetSize() > this->hashtable->GetNumDirectory() * ExtendibleHashtable::EXPAND_THRESHOLD) {
        bool needToEvict = !this->hashtable->ExpandDirectory();
        if (needToEvict) {  // Evict when directory can't be expanded anymore
            this->Evict();
        }
    }

    Page *newPage = new Page(pageId, data);
    this->hashtable->Insert(newPage);
    this->policy->Insert(newPage);
}

void BufferPool::Evict() {
    Page *pageToEvict = this->policy->GetPageToEvict();
    this->hashtable->Remove(pageToEvict);
}
