#include <algorithm>

#include "memtable.h"
#include "test_base.h"

class TestMemtable : public TestBase {
    static bool TestGetMaxSize() {
        auto memtable = new Memtable(3);
        return memtable->GetMaxSize() == 3;
    }

    static bool TestPut() {
        auto memtable = new Memtable(2);
        bool result = true;
        result &= memtable->Put(1, 2);
        result &= memtable->Put(2, 3);
        result &= !memtable->Put(3, 4);
        result &= memtable->GetCurrentSize() == 2;
        return result;
    }

    static bool TestGet() {
        // Set up
        auto memtable = new Memtable(2);
        memtable->Put(1, 2);
        memtable->Put(2, 3);

        // Tests
        bool result = true;
        result &= memtable->Get(1) == 2;
        result &= memtable->Get(2) == 3;
        result &= memtable->Get(3) == Utils::INVALID_VALUE;
        return result;
    }

    static bool TestScan() {
        // Set up
        auto memtable = new Memtable(10);
        for (int i = 0; i < 10; i++) {
            memtable->Put(i, i + 1);
        }

        // Tests
        auto data = memtable->Scan(2, 5);
        if (data.size() != 4) {
            return false;
        }

        bool result = true;
        DataEntry_t expected[4] = {std::make_pair(2, 3), std::make_pair(3, 4), std::make_pair(4, 5),
                                   std::make_pair(5, 6)};
        for (int i = 0; i < 4; i++) {
            result &= expected[i].first == data[i].first;
            result &= expected[i].second == data[i].second;
        }
        return result;
    }

    static bool TestGetAllData() {
        // Set up
        auto memtable = new Memtable(10);
        for (int i = 0; i < 10; i++) {
            memtable->Put(i, i + 1);
        }

        // Tests
        auto data = memtable->GetAllData();
        return data.size() == 10;
    }

    static bool TestReset() {
        // Set up
        auto memtable = new Memtable(10);
        for (int i = 0; i < 10; i++) {
            memtable->Put(i, i + 1);
        }

        // Tests
        if (memtable->GetCurrentSize() != 10) {
            return false;
        }
        memtable->Reset();
        return memtable->GetCurrentSize() == 0;
    }

   public:
    bool RunTests() override {
        bool allTestPassed = true;
        allTestPassed &= assertTrue(TestGetMaxSize, "TestMemtable::TestGetMaxSize");
        allTestPassed &= assertTrue(TestPut, "TestMemtable::TestPut");
        allTestPassed &= assertTrue(TestGet, "TestMemtable::TestGet");
        allTestPassed &= assertTrue(TestScan, "TestMemtable::TestScan");
        allTestPassed &= assertTrue(TestGetAllData, "TestMemtable::TestGetAllData");
        allTestPassed &= assertTrue(TestReset, "TestMemtable::TestReset");
        return allTestPassed;
    }
};
