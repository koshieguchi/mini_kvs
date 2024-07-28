
#ifndef BUFFERPOOL_H
#define BUFFERPOOL_H

#include <cstdint>
#include <string>

#include "eviction_policy.h"
#include "extendible_hashtable.h"

/**
 * Class representing a Buffer Pool in the database.
 */
class BufferPool {
   private:
    // Private data
    ExtendibleHashtable *hashtable;
    EvictionPolicy *policy;

    // Private methods
    void Evict();

   public:
    BufferPool(int minSize, int maxSize, EvictionPolicyType evictionPolicyType);

    ~BufferPool();

    /**
     * Searches for page associated with given pageId in the buffer pool.
     *
     * @param key
     * @param pageKey
     * @return
     */
    std::vector<uint64_t> Get(const std::string &pageId);

    /**
     * Resize the max size of the buffer pool. Triggers eviction if new max size is
     * smaller than current size.
     *
     *   # of evict = ceil(# of current pages - (expansion threshold * newMaxSize))
     *
     * @param newMaxSize
     */
    void Resize(int newMaxSize);

    /**
     * Create a new page with given ID and data and insert it into the buffer pool.
     *
     * @param pageId the ID of the page.
     * @param data the data of the page.
     */
    void Insert(const std::string &pageId, std::vector<uint64_t> &data);
};

#endif  // BUFFERPOOL_H
