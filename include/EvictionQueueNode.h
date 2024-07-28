#ifndef EvictionQueueNode_H
#define EvictionQueueNode_H

#include "Page.h"

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

    void SetNext(EvictionQueueNode *newNext) { this->next = newNext; }

    void SetPrev(EvictionQueueNode *newPrev) { this->prev = newPrev; }
};

#endif  // EvictionQueueNode_H
