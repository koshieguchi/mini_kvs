
#ifndef EVICTION_POLICY_H
#define EVICTION_POLICY_H

#include "page.h"

enum EvictionPolicyType { LRU_t = 0, CLOCK_t = 1 };

/**
 * Abstract class for eviction policies
 */
class EvictionPolicy {
   public:
    /**
     * Update the eviction policy backend when a page in the buffer pool
     * is being accessed.
     *
     * @param accessed_page the Page object accessed.
     */
    virtual void UpdatePageAccessStatus(Page *accessed_page) {};

    /**
     * Update the eviction policy backend when a new page is inserted into
     * the buffer pool.
     *
     * @param page the Page object added.
     */
    virtual void Insert(Page *page) {};

    /**
     * Get the page to evict from buffer pool based on the eviction policy.
     *
     * @return the Page object to be evicted.
     */
    virtual Page *GetPageToEvict() { return nullptr; }
};

#endif  // EVICTION_POLICY_H
