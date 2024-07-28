#ifndef EXTENDIBLE_HASHTABLE_HPP_
#define EXTENDIBLE_HASHTABLE_HPP_

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "./bucket.hpp"
#include "xxhash.h"

class ExtendibleHashtable {
   private:
    // Number of bits used for directory entries. Affects the size of the directory (2^global_depth directory entries).
    int global_depth;

    // Initial and minimum number of bits for directory entries (2^min_depth directory entries).
    int min_depth;

    // Maximum allowed number of bits for directory expansion (2^max_depth directory entries).
    int max_depth;

    // Maximum number of pages a bucket can hold to ensure O(1) access.
    int bucket_max_size;

    // Total number of pages currently stored in the hashtable.
    int size;

    // Mapping from directory entries (bucket IDs) to their corresponding buckets.
    std::map<std::string, Bucket *> buckets;

    // Hash function used to hash the key to a bucket_id.
    std::string hash_function(const std::string &page_id) const;

    // Splits a bucket when it exceeds its capacity.
    void split_bucket(const std::string &target_bucket_id);

    // Merges a bucket with its pair if possible.
    void merge_bucket(const std::string &target_bucket_id);

   public:
    // Expansion threshold, may reference a literature if used.
    // ref: Load Factor and Rehashing, https://www.geeksforgeeks.org/load-factor-and-rehashing/
    constexpr static const float EXPANSION_THRESHOLD = 0.75;

    ExtendibleHashtable(int min_size, int max_size, int bucket_max_size = 1);

    ~ExtendibleHashtable();

    // Get the Page object associated with the given page ID.
    Page *get_page(const std::string &page_id);

    // Insert a new Page object into the hashtable.
    void insert_page(Page *new_page);

    // remove the given Page object from the hashtable.
    void remove_page(Page *page_to_evict);

    // Tries to expand the directory size if possible (e.g., below max size).
    bool expand_directory();

    // Shrink the directory size by first merging all the existing buckets, and then reduce directory size by half.
    void shrink_directory();

    int get_size() const;
    int get_global_depth() const;
    size_t get_num_directory() const;
    int get_num_buckets() const;

    void set_max_size(int max_size);
    void set_min_size(int min_size);

    // Gets the pair bucket ID of the given bucket ID. ("101" would have a pair bucket ID of "001".)
    static std::string get_pair_bucket_id(const std::string &bucket_id);

    std::vector<Page *> get_all_pages();
};

#endif  // EXTENDIBLE_HASHTABLE_HPP_
