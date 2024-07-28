
#include "TestBase.h"
#include "LRU.h"

class TestLRU : public TestBase {

    static bool TestInsert() {
        // Set up
        auto page = new Page("test", {1});
        auto evictPolicy = new LRU();

        // Test
        bool result = true;
        result &= page->GetEvictionQueueNode() == nullptr;
        result &= evictPolicy->GetQueueHead() == nullptr;

        evictPolicy->Insert(page);
        result &= page->GetEvictionQueueNode() != nullptr;
        result &= page->GetEvictionQueueNode() == evictPolicy->GetQueueHead();
        return result;
    }

    static bool TestUpdatePageAccessStatus() {
        // Set up
        auto page1 = new Page("test1", {1});
        auto page2 = new Page("test2", {1});
        auto page3 = new Page("test3", {1});
        auto evictPolicy = new LRU();
        evictPolicy->Insert(page1);
        evictPolicy->Insert(page2);
        evictPolicy->Insert(page3);

        // Test
        bool result = true;

        evictPolicy->UpdatePageAccessStatus(page3);
        // eviction queue: page1 -> page2 -> page3
        result &= evictPolicy->GetQueueHead() == page1->GetEvictionQueueNode();
        result &= evictPolicy->GetQueueHead()->GetNext() == page2->GetEvictionQueueNode();

        evictPolicy->UpdatePageAccessStatus(page2);
        // eviction queue: page1 -> page3 -> page2
        result &= evictPolicy->GetQueueHead() == page1->GetEvictionQueueNode();
        result &= evictPolicy->GetQueueHead()->GetNext() == page3->GetEvictionQueueNode();

        evictPolicy->UpdatePageAccessStatus(page1);
        // eviction queue: page3 -> page2 -> page1
        result &= evictPolicy->GetQueueHead() == page3->GetEvictionQueueNode();

        return result;
    }

    static bool TestGetPageToEvict() {
        // Set up
        auto page1 = new Page("test1", {1});
        auto page2 = new Page("test2", {1});
        auto page3 = new Page("test3", {1});
        auto evictPolicy = new LRU();
        evictPolicy->Insert(page1);
        evictPolicy->Insert(page2);
        evictPolicy->Insert(page3);

        // Test
        bool result = true;
        result &= page1 == evictPolicy->GetPageToEvict();
        result &= evictPolicy->GetQueueHead() == page2->GetEvictionQueueNode();
        result &= evictPolicy->GetQueueHead()->GetNext() == page3->GetEvictionQueueNode();
        result &= evictPolicy->GetQueueHead()->GetNext()->GetNext() == nullptr;
        return result;
    }

public:
    bool RunTests() override {
        bool allTestPassed = true;
        allTestPassed &= assertTrue(TestInsert, "TestLRU::TestInsert");
        allTestPassed &= assertTrue(TestUpdatePageAccessStatus, "TestLRU::TestUpdatePageAccessStatus");
        allTestPassed &= assertTrue(TestGetPageToEvict, "TestLRU::TestGetPageToEvict");
        return allTestPassed;
    }
};
