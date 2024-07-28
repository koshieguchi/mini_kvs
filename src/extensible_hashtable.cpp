#include <cmath>
#include <set>

#include "extendible_hashtable.hpp"
#include "utils.hpp"

ExtendibleHashtable::ExtendibleHashtable(int min_size, int max_size, int bucket_max_size) {
    this->min_depth = std::floor(std::log2(min_size));
    this->global_depth = this->min_depth;
    this->max_depth = std::floor(std::log2(max_size));
    this->bucket_max_size = bucket_max_size;
    this->size = 0;

    // ex) 1<<3 = 8, 1<<4 = 16
    for (int i = 0; i < 1 << this->global_depth; i++) {
        this->buckets.emplace(Utils::get_binary_from_int(i, this->min_depth), new Bucket(this->global_depth));
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

std::string ExtendibleHashtable::hash_function(const std::string &page_id) const {
    void *const buffer = (void *const)page_id.c_str();
    auto hash = XXH64(buffer, page_id.size(), 0);  // 0 is the seed
    return Utils::get_binary_from_int(hash, this->global_depth);
}

bool ExtendibleHashtable::expand_directory() {
    // Can't expand if already reached max directory depth
    if (this->global_depth == this->max_depth) {
        return false;
    }

    this->global_depth++;
    std::map<std::string, Bucket *> temp_bucket(this->buckets.begin(), this->buckets.end());
    for (auto const &[old_id, bucket] : temp_bucket) {
        // Replace old ID with new ID. THe overflowing bucket will have its "1"
        // variant pointing to a new bucket while the rest will have both new IDs
        // pointing to the same bucket.
        this->buckets.emplace("0" + old_id, bucket);
        this->buckets.emplace("1" + old_id, bucket);
        this->buckets.erase(old_id);
    }
    return true;
}

void ExtendibleHashtable::shrink_directory() {
    // Can't shrink if we are still at min directory depth
    if (this->global_depth == this->min_depth) {
        return;
    }

    std::map<std::string, Bucket *> temp_bucket(this->buckets.begin(), this->buckets.end());
    for (auto const &[bucket_id, bucket] : temp_bucket) {
        this->merge_bucket(bucket_id);
    }

    this->global_depth--;
    for (auto const &[old_id, bucket] : temp_bucket) {
        // Replace old ID with new shrunk one and erase the old ID. There are two
        // old IDs that will have same shrunken ID, but since we are using a map,
        // this won't be a problem
        this->buckets[old_id.substr(1)] = bucket;
        this->buckets.erase(old_id);
    }
}

void ExtendibleHashtable::insert_page(Page *page) {
    std::string bucket_id = this->hash_function(page->get_page_id());
    Bucket *bucket = this->buckets.at(bucket_id);
    bucket->insert_page(page);
    this->size++;

    // Split the bucket if the number of pages in the bucket reaches certain directory size threshold
    if (bucket->get_size() > this->bucket_max_size && bucket->get_local_depth() < this->global_depth) {
        this->split_bucket(bucket_id);
    }
}

Page *ExtendibleHashtable::get_page(const std::string &page_id) {
    std::string bucket_id = this->hash_function(page_id);
    auto target_bucket = this->buckets.find(bucket_id);
    if (target_bucket == this->buckets.end()) {
        return nullptr;
    }
    return target_bucket->second->get_page(page_id);
}

void ExtendibleHashtable::remove_page(Page *page_to_evict) {
    std::string bucket_id = this->hash_function(page_to_evict->get_page_id());
    Bucket *bucket = this->buckets.at(bucket_id);
    bucket->remove_page(page_to_evict);
    this->size--;
}

void ExtendibleHashtable::split_bucket(const std::string &bucket_id) {
    Bucket *overflow_bucket = this->buckets.at(bucket_id);
    overflow_bucket->increment_local_depth();

    // Always take "1" + old_id variant to replace with new bucket
    std::string new_bucket_id =
        ((int)bucket_id.length() < this->global_depth) ? "1" + bucket_id : "1" + bucket_id.substr(1);
    std::string pair_id = ExtendibleHashtable::get_pair_bucket_id(new_bucket_id);
    if (this->buckets.at(new_bucket_id) == this->buckets.at(pair_id)) {
        this->buckets[new_bucket_id] = new Bucket(overflow_bucket->get_local_depth());
    }

    // Rehash all the pages in overflowing bucket
    std::forward_list<Page *> pages = overflow_bucket->get_pages();
    this->size -= overflow_bucket->get_size();
    overflow_bucket->clear();
    for (Page *page : pages) {
        this->insert_page(page);
    }
}

void ExtendibleHashtable::merge_bucket(const std::string &bucket_id) {
    Bucket *curr_bucket = this->buckets.at(bucket_id);
    std::string pair_id = get_pair_bucket_id(bucket_id);
    Bucket *pair_bucket = this->buckets.at(pair_id);

    // No need to merge if both directories point at the same bucket
    if (curr_bucket == pair_bucket) {
        return;
    }

    // Move all pages from curr_bucket to pair_bucket, delete the curr_bucket
    // object and reassign current directory to pair_bucket
    std::forward_list<Page *> pages = curr_bucket->get_pages();
    for (Page *page : pages) {
        pair_bucket->insert_page(page);
    }
    pair_bucket->decrement_local_depth();
    delete curr_bucket;
    this->buckets[bucket_id] = pair_bucket;
}

int ExtendibleHashtable::get_global_depth() const { return this->global_depth; }

int ExtendibleHashtable::get_size() const { return this->size; }

size_t ExtendibleHashtable::get_num_directory() const { return this->buckets.size(); }

int ExtendibleHashtable::get_num_buckets() const {
    int count = 0;
    std::set<Bucket *> bucket_set;
    for (auto const &[_, bucket] : this->buckets) {
        if (bucket_set.count(bucket) == 0) {
            bucket_set.insert(bucket);
            count++;
        }
    }
    return count;
}

void ExtendibleHashtable::set_max_size(int max_size) {
    this->max_depth = std::floor(std::log2(max_size));

    // min_depth is always smaller (or equal to) max_depth
    if (this->min_depth > this->max_depth) {
        this->min_depth = this->max_depth;
    }

    // Shrink directory if global_depth is bigger than max_depth
    while (this->global_depth > this->max_depth) {
        this->shrink_directory();
    }
}

void ExtendibleHashtable::set_min_size(int min_size) {
    this->min_depth = std::floor(std::log2(min_size));
    if (this->max_depth < this->min_depth) {
        // Increment max_depth if min_depth is bigger
        this->max_depth = this->min_depth;
    }
}

// Generates the pair bucket ID for a given bucket ID. The pair bucket ID is
// obtained by flipping the first bit of the given bucket ID, while keeping the
// rest of the ID unchanged.
std::string ExtendibleHashtable::get_pair_bucket_id(const std::string &bucket_id) {
    std::string pair_bucket_id = bucket_id;

    // Flip the first bit: if it's '0', change to '1', and vice versa.
    pair_bucket_id[0] = (bucket_id[0] == '0') ? '1' : '0';

    return pair_bucket_id;
}

std::vector<Page *> ExtendibleHashtable::get_all_pages() {
    std::vector<Page *> all_pages;
    for (auto &bucket_entry : buckets) {
        auto pages = bucket_entry.second->get_pages();
        all_pages.insert(all_pages.end(), pages.begin(), pages.end());
    }
    return all_pages;
}
