
#ifndef EXTENDIBLE_HASHTABLE_H
#define EXTENDIBLE_HASHTABLE_H

#include <cstdint>
#include <vector>

#include "bucket.h"
#include "xxhash.h"

/**
 * Class representing a Extendible Hashtable data structure.
 */
class ExtendibleHashtable {
   private:
    // Current number of bits used to create directory entries (i.e. 2^global_depth directory entries).
    int global_depth;
    // Min and initial number of bits used to create directory entries (i.e. 2^min_depth directory entries).
    int min_depth;
    // Max number of bits allowed to be used to expand the number of directory entries (i.e. 2^max_depth directory
    // entries).
    int max_depth;
    // Max number of pages mapped to a bucket to make sure we maintain a O(1) access.
    int bucket_max_size;
    // Total number of pages mapped to this extendible hashtable.
    int size;
    // Map of dir entries (i.e. bucket_id) to their corresponding bucket.
    std::map<std::string, Bucket *> buckets;

    // Private methods
    [[nodiscard]] std::string Hash(const std::string &page_id) const;

    void Split(const std::string &bucket_id);

    void Merge(const std::string &bucket_id);

   public:
    constexpr static const float EXPAND_THRESHOLD = 0.8;

    ExtendibleHashtable(int min_size, int max_size, int bucket_max_size = 1);

    ~ExtendibleHashtable();

    /**
     * Fetch the Page object given it's page ID.
     *
     * @param page_id
     * @return the Page object associated with the ID.
     */
    Page *Get(const std::string &page_id);

    /**
     * Insert the given new Page object into the hash table.
     *
     * @param page Page object to be inserted.
     */
    void Insert(Page *page);

    /**
     * Remove the given Page object from the hash table.
     *
     * @param page_to_evict Page object to be removed.
     */
    void Remove(Page *page_to_evict);

    /**
     * Tries to expand the directory size if possible (e.g., below max size).
     *
     * @return true if expansion was successful, false otherwise.
     */
    bool ExpandDirectory();

    /**
     * Shrink the directory size by first merging all the existing
     * buckets, and then reduce directory size by half.
     */
    void Shrink();

    [[nodiscard]] int GetGlobalDepth() const;

    [[nodiscard]] int GetSize() const;

    [[nodiscard]] size_t GetNumDirectory() const;

    /**
     * Returns the number of buckets in the hashtable
     *
     * Note: this method should only be used for testing purposes only.
     */
    [[nodiscard]] int GetNumBuckets() const;

    void SetMaxSize(int max_size);

    void SetMinSize(int min_size);

    /**
     * Gets the pair bucket ID of the given bucket ID.
     *
     * For example, "101" would have a pair bucket ID of "001".
     *
     * @param bucket_id the bucket ID
     * @return the pair bucket ID
     */
    static std::string GetPairBucket_id(const std::string &bucket_id);
};

#endif  // EXTENDIBLE_HASHTABLE_H
