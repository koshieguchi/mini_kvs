#ifndef LRU_H
#define LRU_H

#include <vector>

#include "eviction_policy.h"
#include "eviction_queue_node.h"

/**
 * Class representing LRU eviction policy for buffer pool.
 */
class LRU : public EvictionPolicy {
   private:
    EvictionQueueNode *eviction_queue_head;
    EvictionQueueNode *most_recent;

   public:
    LRU();

    /**
     * We don't do any de-allocation here since each EvictionQueueNode is associated
     * with a Page object, and we will let the Page object do the de-allocation for us.
     */
    ~LRU() = default;

    void Insert(Page *page) override;

    void UpdatePageAccessStatus(Page *accessed_page) override;

    Page *GetPageToEvict() override;

    EvictionQueueNode *GetQueueHead();
};

#endif  // LRU_H
