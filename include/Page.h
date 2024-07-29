
#ifndef PAGE_H
#define PAGE_H

#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "eviction_queue_node.h"

class EvictionQueueNode;

/**
 * Class representing a database Page.
 */
class Page {
   private:
    std::string page_id;
    std::vector<uint64_t> data;
    EvictionQueueNode *eviction_node;  // Used in LRU
    bool accessBit;                    // Used in Clock
   public:
    /**
     * Constructor for a Page object
     *
     * @param page_id the ID of the page.
     * @param data the key-value data stored in the page.
     * @param eviction_node the eviction queue linked list node. Used for LRU eviction policy.
     */
    Page(const std::string &page_id, std::vector<uint64_t> data, EvictionQueueNode *eviction_node = nullptr) {
        this->page_id = page_id;
        this->data = std::move(data);
        this->eviction_node = eviction_node;
        this->accessBit = false;
    }

    ~Page() { delete this->eviction_node; }

    /**
     * Get the ID of the current page.
     */
    std::string GetPageId() { return this->page_id; }

    /**
     * Get the access bit of the page. Used for CLOCK eviction policy.
     */
    [[nodiscard]] bool GetAccessBit() const { return this->accessBit; }

    /**
     * Get the eviction queue linked list node of the page. Used for LRU eviction policy.
     */
    EvictionQueueNode *GetEvictionQueueNode() { return this->eviction_node; }

    /**
     * Set the eviction queue linked list node of the page. Used for LRU eviction policy.
     */
    void SetEvictionQueueNode(EvictionQueueNode *new_eviction_node) { this->eviction_node = new_eviction_node; }

    /**
     * Set the access bit of the page. Used for CLOCK eviction policy.
     */
    void SetAccessBit(int accessStatus) { this->accessBit = accessStatus; }

    /**
     * Get all the key-value data within the page. The keys are on even indices while
     * values are on odd indices.
     *
     * @return a vector containing all key-value pairs.
     */
    std::vector<uint64_t> GetData() { return this->data; }
};

#endif  // PAGE_H
