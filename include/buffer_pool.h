
#ifndef BUFFER_POOL_H
#define BUFFER_POOL_H

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
    BufferPool(int min_size, int max_size, EvictionPolicyType eviction_policy_type);

    ~BufferPool();

    /**
     * Searches for page associated with given page_id in the buffer pool.
     *
     * @param page_id
     * @return
     */
    std::vector<uint64_t> Get(const std::string &page_id);

    /**
     * Resize the max size of the buffer pool. Triggers eviction if new max size is
     * smaller than current size.
     *
     *   # of evict = ceil(# of current pages - (expansion threshold * new_max_size))
     *
     * @param new_max_size
     */
    void Resize(int new_max_size);

    /**
     * Create a new page with given ID and data and insert it into the buffer pool.
     *
     * @param page_id the ID of the page.
     * @param data the data of the page.
     */
    void Insert(const std::string &page_id, std::vector<uint64_t> &data);
};

#endif  // BUFFER_POOL_H
