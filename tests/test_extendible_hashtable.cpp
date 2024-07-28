
#include "../include/extendible_hashtable.h"
#include "test_base.h"

class TestExtendibleHashtable : public TestBase {
   public:
    static bool TestConstructor() {
        bool result = true;
        auto hashtable = new ExtendibleHashtable(2, 8);
        result &= hashtable->GetSize() == 0;
        result &= hashtable->GetGlobalDepth() == 1;
        result &= hashtable->GetNumDirectory() == 2;

        return result;
    }

    static bool TestInsert() {
        // Set up
        auto hashtable = new ExtendibleHashtable(2, 8, 1);
        auto page1 = new Page("test/1.sst", std::vector<uint64_t>{1, 2});

        // Tests
        bool result = true;
        hashtable->Insert(page1);
        result &= hashtable->GetSize() == 1;
        return result;
    }

    static bool TestGet() {
        // Set up
        auto hashtable = new ExtendibleHashtable(2, 8);
        auto page = new Page("test/1.sst", std::vector<uint64_t>{1, 2});
        hashtable->Insert(page);

        // Tests
        bool result = true;
        std::vector<uint64_t> data;
        Page *fetchedPage = hashtable->Get("test/2.sst");
        result &= fetchedPage == nullptr;

        fetchedPage = hashtable->Get("test/1.sst");
        result &= fetchedPage != nullptr;
        result &= fetchedPage->GetData().size() == 2;
        result &= fetchedPage->GetData()[1] == 2;
        return result;
    }

    static bool TestRemove() {
        // Set up (both page1 and page2 will be hashed to 1 bucket, while page3 into another)
        auto hashtable = new ExtendibleHashtable(2, 8);
        auto page1 = new Page("test/1.sst", std::vector<uint64_t>{1, 2});
        auto page2 = new Page("test/1.sst", std::vector<uint64_t>{1, 2});
        auto page3 = new Page("test/1.sst", std::vector<uint64_t>{3, 4});
        hashtable->Insert(page1);
        hashtable->Insert(page2);
        hashtable->Insert(page3);

        // Tests
        bool result = true;
        hashtable->Remove(page1);
        result &= hashtable->GetSize() == 2;

        hashtable->Remove(page3);
        result &= hashtable->GetSize() == 1;
        return result;
    }

    static bool TestShrink() {
        // Set up (both page1 and page2 will be hashed to 1 bucket, while page3 into another)
        auto hashtable = new ExtendibleHashtable(4, 8, 1);
        auto page1 = new Page("test/1.sst", std::vector<uint64_t>{1, 2});
        auto page2 = new Page("test/3.sst", std::vector<uint64_t>{3, 4});
        hashtable->Insert(page1);
        hashtable->Insert(page2);

        // Tests
        bool result = true;
        result &= hashtable->GetSize() == 2;
        result &= hashtable->GetGlobalDepth() == 2;
        result &= hashtable->GetNumDirectory() == 4;
        result &= hashtable->GetNumBuckets() == 4;

        hashtable->Shrink();  // Shrinking when global depth is at min depth won't do anything
        result &= hashtable->GetSize() == 2;
        result &= hashtable->GetGlobalDepth() == 2;
        result &= hashtable->GetNumDirectory() == 4;
        result &= hashtable->GetNumBuckets() == 4;

        // Shrinking after making min depth smaller (triggered by reducing max size) should reduce
        // directory size and reducing max size below current global depth should trigger shrink.
        hashtable->SetMaxSize(2);
        result &= hashtable->GetSize() == 2;
        result &= hashtable->GetGlobalDepth() == 1;
        result &= hashtable->GetNumDirectory() == 2;
        result &= hashtable->GetNumBuckets() == 2;
        return result;
    }

    bool RunTests() override {
        bool result = true;
        result &= assertTrue(TestConstructor, "TestExtendibleHashtable::TestConstructor");
        result &= assertTrue(TestInsert, "TestExtendibleHashtable::TestInsert");
        result &= assertTrue(TestGet, "TestExtendibleHashtable::TestGet");
        result &= assertTrue(TestRemove, "TestExtendibleHashtable::TestRemove");
        result &= assertTrue(TestShrink, "TestExtendibleHashtable::TestShrink");
        return result;
    }
};
