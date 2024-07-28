
#include "clock.h"
#include "test_base.h"

class TestClock : public TestBase {
    static bool TestInsert() {
        // Set up
        auto page = new Page("test", {1});
        auto evictPolicy = new Clock();

        // Test
        bool result = true;
        evictPolicy->Insert(page);
        result &= page->GetAccessBit() == 0;
        return result;
    }

    static bool TestUpdatePageAccessStatus() {
        // Set up
        auto page = new Page("test1", {1});
        auto evictPolicy = new Clock();
        evictPolicy->Insert(page);

        // Test
        bool result = true;
        result &= page->GetAccessBit() == 0;
        evictPolicy->UpdatePageAccessStatus(page);
        result &= page->GetAccessBit() == 1;
        return result;
    }

    static bool TestGetPageToEvict() {
        // Set up
        auto page1 = new Page("test1", {1});
        auto page2 = new Page("test2", {1});
        auto page3 = new Page("test3", {1});
        auto evictPolicy = new Clock();
        evictPolicy->Insert(page1);
        evictPolicy->Insert(page2);
        evictPolicy->Insert(page3);

        // Test
        bool result = true;
        evictPolicy->UpdatePageAccessStatus(page1);
        evictPolicy->UpdatePageAccessStatus(page2);
        result &= page3 == evictPolicy->GetPageToEvict();
        result &= page1->GetAccessBit() == 1;
        result &= page2->GetAccessBit() == 1;
        result &= page2 == evictPolicy->GetPageToEvict();
        result &= page1->GetAccessBit() == 0;
        result &= page1 == evictPolicy->GetPageToEvict();
        return result;
    }

   public:
    bool RunTests() override {
        bool allTestPassed = true;
        allTestPassed &= assertTrue(TestInsert, "TestClock::TestInsert");
        allTestPassed &= assertTrue(TestUpdatePageAccessStatus, "TestClock::TestUpdatePageAccessStatus");
        allTestPassed &= assertTrue(TestGetPageToEvict, "TestClock::TestGetPageToEvict");
        return allTestPassed;
    }
};
