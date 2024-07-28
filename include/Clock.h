#ifndef Clock_H
#define Clock_H

#include <list>
#include <vector>

#include "EvictionPolicy.h"
#include "page.h"

/**
 * Class representing CLOCK eviction policy for buffer pool.
 */
class Clock : public EvictionPolicy {
   private:
    std::list<Page *> pages;
    int handle;

   public:
    Clock();

    void Insert(Page *page) override;

    void UpdatePageAccessStatus(Page *accessedPage) override;

    Page *GetPageToEvict() override;
};

#endif  // Clock_H
