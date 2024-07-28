#ifndef LRU_H
#define LRU_H

#include <vector>

#include "EvictionPolicy.h"
#include "EvictionQueueNode.h"

/**
 * Class representing LRU eviction policy for buffer pool.
 */
class LRU : public EvictionPolicy {
   private:
    EvictionQueueNode *evictionQueueHead;
    EvictionQueueNode *mostRecent;

   public:
    LRU();

    /**
     * We don't do any de-allocation here since each EvictionQueueNode is associated
     * with a Page object, and we will let the Page object do the de-allocation for us.
     */
    ~LRU() = default;

    void Insert(Page *page) override;

    void UpdatePageAccessStatus(Page *accessedPage) override;

    Page *GetPageToEvict() override;

    EvictionQueueNode *GetQueueHead();
};

#endif  // LRU_H
