#ifndef EvictionQueueNode_H
#define EvictionQueueNode_H

#include "page.h"

class Page;

class EvictionQueueNode {
   private:
    Page *page;
    EvictionQueueNode *next;
    EvictionQueueNode *prev;

   public:
    explicit EvictionQueueNode(Page *page, EvictionQueueNode *next, EvictionQueueNode *prev)
        : page(page), next(next), prev(prev) {};

    Page *GetPage() { return this->page; }

    EvictionQueueNode *GetNext() { return this->next; }

    EvictionQueueNode *GetPrev() { return this->prev; }

    void SetNext(EvictionQueueNode *new_next) { this->next = new_next; }

    void SetPrev(EvictionQueueNode *new_prev) { this->prev = new_prev; }
};

#endif  // EvictionQueueNode_H
