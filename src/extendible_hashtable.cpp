
#include "extendible_hashtable.h"

#include <cmath>
#include <set>

#include "utils.h"

ExtendibleHashtable::ExtendibleHashtable(int minSize, int maxSize, int bucketMaxSize) {
    // globalDepth always changes between minDepth <= globalDepth <= maxDepth.
    this->minDepth = std::floor(std::log2(minSize));
    this->globalDepth = this->minDepth;
    this->maxDepth = std::floor(std::log2(maxSize));
    this->bucketMaxSize = bucketMaxSize;
    this->size = 0;

    for (int i = 0; i < 1 << this->globalDepth; i++) {
        this->buckets.emplace(Utils::GetBinaryFromInt(i, this->minDepth), new Bucket(this->globalDepth));
    }
}

ExtendibleHashtable::~ExtendibleHashtable() {
    // First gather buckets into set as some directory may point to same bucket
    std::set<Bucket *> erasedBuckets;
    for (auto const &[_, bucket] : this->buckets) {
        erasedBuckets.insert(bucket);
    }

    for (auto bucket : erasedBuckets) {
        delete bucket;
    }
    this->buckets.clear();
}

std::string ExtendibleHashtable::Hash(const std::string &pageId) const {
    void *const buffer = (void *const)pageId.c_str();
    XXH64_hash_t hash = XXH64(buffer, pageId.size(), 1);
    return Utils::GetBinaryFromInt(hash, this->globalDepth);
}

bool ExtendibleHashtable::ExpandDirectory() {
    // Can't expand if we already reached max directory depth
    if (this->globalDepth == this->maxDepth) {
        return false;
    }

    this->globalDepth++;
    std::map<std::string, Bucket *> iterCopy(this->buckets.begin(), this->buckets.end());
    for (auto const &[oldId, bucket] : iterCopy) {
        // Replace old ID with new expanded one. The overflowing bucket will have its "1" variant
        // pointing to a new bucket while the rest will have both new IDs point to the same bucket
        this->buckets.emplace("0" + oldId, bucket);
        this->buckets.emplace("1" + oldId, bucket);
        this->buckets.erase(oldId);
    }
    return true;
}

void ExtendibleHashtable::Shrink() {
    // Can't shrink if we are still at min directory depth
    if (this->globalDepth == this->minDepth) {
        return;
    }

    std::map<std::string, Bucket *> iterCopy(this->buckets.begin(), this->buckets.end());
    for (auto const &[bucketId, bucket] : iterCopy) {
        this->Merge(bucketId);
    }

    this->globalDepth--;
    for (auto const &[oldId, bucket] : iterCopy) {
        // Replace old ID with new shrunk one and erase the old ID. There are two old IDs that will
        // have same shrunken ID, but since we are using a map, this won't be a problem
        this->buckets[oldId.substr(1)] = bucket;
        this->buckets.erase(oldId);
    }
}

void ExtendibleHashtable::Insert(Page *page) {
    std::string bucketId = this->Hash(page->GetPageId());
    Bucket *bucket = this->buckets.at(bucketId);
    bucket->Insert(page);
    this->size++;

    // Split the bucket if the number of pages in the bucket reaches certain directory size threshold
    if (bucket->GetSize() > this->bucketMaxSize && bucket->GetLocalDepth() < this->globalDepth) {
        this->Split(bucketId);
    }
}

Page *ExtendibleHashtable::Get(const std::string &pageId) {
    std::string bucketId = this->Hash(pageId);
    auto targetBucket = this->buckets.find(bucketId);
    if (targetBucket == this->buckets.end()) {
        return nullptr;
    }
    return targetBucket->second->Get(pageId);
}

void ExtendibleHashtable::Remove(Page *pageToEvict) {
    std::string bucketId = this->Hash(pageToEvict->GetPageId());
    Bucket *bucket = this->buckets.at(bucketId);
    bucket->Remove(pageToEvict);
    this->size--;
}

void ExtendibleHashtable::Split(const std::string &bucketId) {
    Bucket *overflowBucket = this->buckets.at(bucketId);
    overflowBucket->IncreaseLocalDepth();

    // Always take "1" + oldId variant to replace with new bucket
    std::string newBucketId = (bucketId.length() < this->globalDepth) ? "1" + bucketId : "1" + bucketId.substr(1);
    std::string pairId = ExtendibleHashtable::GetPairBucketId(newBucketId);
    if (this->buckets.at(newBucketId) == this->buckets.at(pairId)) {
        this->buckets[newBucketId] = new Bucket(overflowBucket->GetLocalDepth());
    }

    // Re-hash all the pages in overflowing bucket
    std::forward_list<Page *> pages = overflowBucket->GetPages();
    this->size -= overflowBucket->GetSize();
    overflowBucket->Clear();
    for (Page *page : pages) {
        this->Insert(page);
    }
}

void ExtendibleHashtable::Merge(const std::string &bucketId) {
    Bucket *currBucket = this->buckets.at(bucketId);
    std::string pairId = GetPairBucketId(bucketId);
    Bucket *pairBucket = this->buckets.at(pairId);

    // No need to merge if both directories point at the same bucket
    if (currBucket == pairBucket) {
        return;
    }

    // Move all pages from currBucket to pairBucket, delete the currBucket object
    // and reassign current directory to pairBucket
    std::forward_list<Page *> pages = currBucket->GetPages();
    for (Page *page : pages) {
        pairBucket->Insert(page);
    }
    pairBucket->DecreaseLocalDepth();
    delete currBucket;
    this->buckets[bucketId] = pairBucket;
}

int ExtendibleHashtable::GetGlobalDepth() const { return this->globalDepth; }

int ExtendibleHashtable::GetSize() const { return this->size; }

size_t ExtendibleHashtable::GetNumDirectory() const { return this->buckets.size(); }

int ExtendibleHashtable::GetNumBuckets() const {
    int count = 0;
    std::set<Bucket *> bucketSet;
    for (auto const &[_, bucket] : this->buckets) {
        if (bucketSet.count(bucket) == 0) {
            bucketSet.insert(bucket);
            count++;
        }
    }
    return count;
}

void ExtendibleHashtable::SetMaxSize(int maxSize) {
    this->maxDepth = std::floor(std::log2(maxSize));
    if (this->minDepth > this->maxDepth) {  // Reduce minDepth if maxDepth is smaller
        this->minDepth = this->maxDepth;
    }
    while (this->globalDepth > this->maxDepth) {
        this->Shrink();
    }
}

void ExtendibleHashtable::SetMinSize(int minSize) {
    this->minDepth = std::floor(std::log2(minSize));
    if (this->maxDepth < this->minDepth) {  // Increment maxDepth if minDepth is bigger
        this->maxDepth = this->minDepth;
    }
}

std::string ExtendibleHashtable::GetPairBucketId(const std::string &bucketId) {
    std::string suffix = bucketId.substr(1);
    return (bucketId.at(0) == '0') ? "1" + suffix : "0" + suffix;
}
