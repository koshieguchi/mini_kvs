
#include "extendible_hashtable.h"

#include <cmath>
#include <set>

#include "utils.h"

ExtendibleHashtable::ExtendibleHashtable(int min_size, int max_size, int bucket_max_size) {
    // global_depth always changes between min_depth <= global_depth <= max_depth.
    this->min_depth = std::floor(std::log2(min_size));
    this->global_depth = this->min_depth;
    this->max_depth = std::floor(std::log2(max_size));
    this->bucket_max_size = bucket_max_size;
    this->size = 0;

    for (int i = 0; i < 1 << this->global_depth; i++) {
        this->buckets.emplace(Utils::GetBinaryFromInt(i, this->min_depth), new Bucket(this->global_depth));
    }
}

ExtendibleHashtable::~ExtendibleHashtable() {
    // First gather buckets into set as some directory may point to same bucket
    std::set<Bucket *> erased_buckets;
    for (auto const &[_, bucket] : this->buckets) {
        erased_buckets.insert(bucket);
    }

    for (auto bucket : erased_buckets) {
        delete bucket;
    }
    this->buckets.clear();
}

std::string ExtendibleHashtable::Hash(const std::string &page_id) const {
    void *const buffer = (void *const)page_id.c_str();
    XXH64_hash_t hash = XXH64(buffer, page_id.size(), 1);
    return Utils::GetBinaryFromInt(hash, this->global_depth);
}

bool ExtendibleHashtable::ExpandDirectory() {
    // Can't expand if we already reached max directory depth
    if (this->global_depth == this->max_depth) {
        return false;
    }

    this->global_depth++;
    std::map<std::string, Bucket *> iterCopy(this->buckets.begin(), this->buckets.end());
    for (auto const &[old_id, bucket] : iterCopy) {
        // Replace old ID with new expanded one. The overflowing bucket will have its "1" variant
        // pointing to a new bucket while the rest will have both new IDs point to the same bucket
        this->buckets.emplace("0" + old_id, bucket);
        this->buckets.emplace("1" + old_id, bucket);
        this->buckets.erase(old_id);
    }
    return true;
}

void ExtendibleHashtable::Shrink() {
    // Can't shrink if we are still at min directory depth
    if (this->global_depth == this->min_depth) {
        return;
    }

    std::map<std::string, Bucket *> iterCopy(this->buckets.begin(), this->buckets.end());
    for (auto const &[bucket_id, bucket] : iterCopy) {
        this->Merge(bucket_id);
    }

    this->global_depth--;
    for (auto const &[old_id, bucket] : iterCopy) {
        // Replace old ID with new shrunk one and erase the old ID. There are two old IDs that will
        // have same shrunken ID, but since we are using a map, this won't be a problem
        this->buckets[old_id.substr(1)] = bucket;
        this->buckets.erase(old_id);
    }
}

void ExtendibleHashtable::Insert(Page *page) {
    std::string bucket_id = this->Hash(page->GetPageId());
    Bucket *bucket = this->buckets.at(bucket_id);
    bucket->Insert(page);
    this->size++;

    // Split the bucket if the number of pages in the bucket reaches certain directory size threshold
    if (bucket->GetSize() > this->bucket_max_size && bucket->GetLocalDepth() < this->global_depth) {
        this->Split(bucket_id);
    }
}

Page *ExtendibleHashtable::Get(const std::string &page_id) {
    std::string bucket_id = this->Hash(page_id);
    auto target_bucket = this->buckets.find(bucket_id);
    if (target_bucket == this->buckets.end()) {
        return nullptr;
    }
    return target_bucket->second->Get(page_id);
}

void ExtendibleHashtable::Remove(Page *page_to_evict) {
    std::string bucket_id = this->Hash(page_to_evict->GetPageId());
    Bucket *bucket = this->buckets.at(bucket_id);
    bucket->Remove(page_to_evict);
    this->size--;
}

void ExtendibleHashtable::Split(const std::string &bucket_id) {
    Bucket *overflowBucket = this->buckets.at(bucket_id);
    overflowBucket->IncreaseLocalDepth();

    // Always take "1" + old_id variant to replace with new bucket
    std::string newBucket_id = (bucket_id.length() < this->global_depth) ? "1" + bucket_id : "1" + bucket_id.substr(1);
    std::string pairId = ExtendibleHashtable::GetPairBucket_id(newBucket_id);
    if (this->buckets.at(newBucket_id) == this->buckets.at(pairId)) {
        this->buckets[newBucket_id] = new Bucket(overflowBucket->GetLocalDepth());
    }

    // Re-hash all the pages in overflowing bucket
    std::forward_list<Page *> pages = overflowBucket->GetPages();
    this->size -= overflowBucket->GetSize();
    overflowBucket->Clear();
    for (Page *page : pages) {
        this->Insert(page);
    }
}

void ExtendibleHashtable::Merge(const std::string &bucket_id) {
    Bucket *currBucket = this->buckets.at(bucket_id);
    std::string pairId = GetPairBucket_id(bucket_id);
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
    this->buckets[bucket_id] = pairBucket;
}

int ExtendibleHashtable::GetGlobalDepth() const { return this->global_depth; }

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

void ExtendibleHashtable::SetMaxSize(int max_size) {
    this->max_depth = std::floor(std::log2(max_size));
    if (this->min_depth > this->max_depth) {  // Reduce min_depth if max_depth is smaller
        this->min_depth = this->max_depth;
    }
    while (this->global_depth > this->max_depth) {
        this->Shrink();
    }
}

void ExtendibleHashtable::SetMinSize(int min_size) {
    this->min_depth = std::floor(std::log2(min_size));
    if (this->max_depth < this->min_depth) {  // Increment max_depth if min_depth is bigger
        this->max_depth = this->min_depth;
    }
}

std::string ExtendibleHashtable::GetPairBucket_id(const std::string &bucket_id) {
    std::string suffix = bucket_id.substr(1);
    return (bucket_id.at(0) == '0') ? "1" + suffix : "0" + suffix;
}
