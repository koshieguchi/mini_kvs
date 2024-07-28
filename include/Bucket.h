
#ifndef BUCKET_H
#define BUCKET_H

#include <cstdint>
#include <forward_list>

#include "Page.h"

/**
 * Class representing a Bucket in a Extendible Hashtable data structure.
 */
class Bucket {
   private:
    std::forward_list<Page *> pages;
    // Number of bits used by the bucket so far, out of the total globalDepth bits of the hashtable.
    int localDepth;
    // Number of pages mapped to this bucket.
    int size;

   public:
    /**
     * Constructor for a Bucket object.
     *
     * @param depth the depth (e.g., number of bits) of the bucket.
     */
    explicit Bucket(int depth);

    ~Bucket();

    /**
     * Searches for value associated with given key in the bucket.
     *
     * @param key
     * @return
     */
    Page *Get(const std::string &pageId);

    /**
     * Insert a new Page object into the bucket chain.
     *
     * @param newPage
     */
    void Insert(Page *newPage);

    /**
     * Remove the given Page object from the bucket chain.
     *
     * @param pageToRemove
     */
    void Remove(Page *pageToRemove);

    /**
     * Get the number of pages mapped to current bucket.
     */
    [[nodiscard]] int GetSize() const;

    /**
     * Get the local depth (number of bits used) of the current bucket.
     */
    [[nodiscard]] int GetLocalDepth() const;

    /**
     * Get all the Page objects in the bucket.
     */
    std::forward_list<Page *> GetPages();

    /**
     * Increment the local depth of the bucket.
     */
    void IncreaseLocalDepth();

    /**
     * Decrement the local depth of the bucket.
     */
    void DecreaseLocalDepth();

    /**
     * Clear out all the pages stored within the bucket.
     */
    void Clear();
};

#endif  // BUCKET_H
