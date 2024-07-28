
#ifndef EXTENDIBLEHASHTABLE_H
#define EXTENDIBLEHASHTABLE_H

#include <cstdint>
#include <vector>

#include "bucket.h"
#include "xxhash.h"

/**
 * Class representing a Extendible Hashtable data structure.
 */
class ExtendibleHashtable {
   private:
    // Current number of bits used to create directory entries (i.e. 2^globalDepth directory entries).
    int globalDepth;
    // Min and initial number of bits used to create directory entries (i.e. 2^minDepth directory entries).
    int minDepth;
    // Max number of bits allowed to be used to expand the number of directory entries (i.e. 2^maxDepth directory
    // entries).
    int maxDepth;
    // Max number of pages mapped to a bucket to make sure we maintain a O(1) access.
    int bucketMaxSize;
    // Total number of pages mapped to this extendible hashtable.
    int size;
    // Map of dir entries (i.e. bucketId) to their corresponding bucket.
    std::map<std::string, Bucket *> buckets;

    // Private methods
    [[nodiscard]] std::string Hash(const std::string &pageId) const;

    void Split(const std::string &bucketId);

    void Merge(const std::string &bucketId);

   public:
    constexpr static const float EXPAND_THRESHOLD = 0.8;

    ExtendibleHashtable(int minSize, int maxSize, int bucketMaxSize = 1);

    ~ExtendibleHashtable();

    /**
     * Fetch the Page object given it's page ID.
     *
     * @param pageId
     * @return the Page object associated with the ID.
     */
    Page *Get(const std::string &pageId);

    /**
     * Insert the given new Page object into the hash table.
     *
     * @param page Page object to be inserted.
     */
    void Insert(Page *page);

    /**
     * Remove the given Page object from the hash table.
     *
     * @param pageToEvict Page object to be removed.
     */
    void Remove(Page *pageToEvict);

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

    void SetMaxSize(int maxSize);

    void SetMinSize(int minSize);

    /**
     * Gets the pair bucket ID of the given bucket ID.
     *
     * For example, "101" would have a pair bucket ID of "001".
     *
     * @param bucketId the bucket ID
     * @return the pair bucket ID
     */
    static std::string GetPairBucketId(const std::string &bucketId);
};

#endif  // EXTENDIBLEHASHTABLE_H
