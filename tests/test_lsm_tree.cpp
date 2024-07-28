#include <filesystem>
#include <string>
#include <vector>

#include "lsm_tree.h"
#include "test_base.h"

namespace fs = std::filesystem;

uint64_t GetData(uint64_t start, uint64_t end, uint64_t value, std::vector<DataEntry_t> &data, int increment) {
    uint64_t valuesSum = 0;
    for (uint64_t i = start; i < end; i += increment) {
        for (uint64_t t = i * 256; t < (i + 1) * 256; t++) {
            if (value == Utils::DELETED_KEY_VALUE) {
                data.emplace_back(t, value);
            } else {
                data.emplace_back(t, t * value);
                valuesSum += t * value;
            }
        }
    }
    return valuesSum;
}

class TestLSMTree : public TestBase {
    // Memtable with size 512 pages
    static const SearchType searchType = SearchType::B_TREE_SEARCH;
    static const int bloomFilterBitsPerEntry = 10;
    static const int inputBufferCapacity = 8;
    static const int outputBufferCapacity = 256;
    inline static std::string kvsDirPath = "./kvs_lsm_tree";

    static LSMTree *Setup() {
        if (!fs::exists(kvsDirPath)) {
            bool createDirRes = fs::create_directories(kvsDirPath);
            if (!createDirRes) {
                return nullptr;
            }
        }
        return new LSMTree(bloomFilterBitsPerEntry, inputBufferCapacity, outputBufferCapacity);
    }

    static void WriteDataToLSMTree(LSMTree *lsmTree, std::vector<DataEntry_t> &data) {
        if (lsmTree->GetLevels().empty()) {
            auto *firstLevel = new Level(0, bloomFilterBitsPerEntry, inputBufferCapacity, outputBufferCapacity);
            lsmTree->AddLevel(firstLevel);
        }
        lsmTree->GetLevels()[0]->WriteDataToLevel(data, searchType, kvsDirPath);
    }

    /**
     * Expect the memtable data to be written to the first lavel of LSM tree.
     */
    static bool TestWriteMemtableData() {
        LSMTree *lsmTree = Setup();
        if (!lsmTree) {
            return false;
        }

        // 1. Set up data
        std::vector<DataEntry_t> data;
        GetData(0, 512, 10, data, 1);

        // 2. Run and check expected values
        bool result = true;

        lsmTree->WriteMemtableData(data, searchType, kvsDirPath);
        std::string fileName = kvsDirPath + "/level0-0.sst";
        result &= lsmTree->GetLevels().size() == 1;
        result &= lsmTree->GetLevels()[0]->GetSSTFiles().size() == 1;
        result &= lsmTree->GetLevels()[0]->GetSSTFiles()[0]->GetFileName() == fileName;

        // 3. Clean up
        fs::remove_all(kvsDirPath);
        return result;
    }

    /**
     * Expect the levels to sort and flush to next level when there are two sst files in them.
     */
    static bool TestMaintainLevelsCapacityAndCompact() {
        LSMTree *lsmTree = Setup();
        if (!lsmTree) {
            return false;
        }

        // 1. Set up data by writing 2 sst files to first level
        std::vector<DataEntry_t> data1;
        GetData(0, 512, 10, data1, 1);
        WriteDataToLSMTree(lsmTree, data1);

        std::vector<DataEntry_t> data2;
        GetData(512, 2 * 512 + 1, 10, data2, 1);
        WriteDataToLSMTree(lsmTree, data2);

        // 2. Run and check expected values
        bool result = true;

        lsmTree->MaintainLevelCapacityAndCompact(lsmTree->GetLevels()[0], kvsDirPath);
        std::string fileName = kvsDirPath + "/level1-0.sst";
        result &= lsmTree->GetLevels().size() == 2;
        result &= lsmTree->GetLevels()[0]->GetSSTFiles().empty();
        result &= lsmTree->GetLevels()[1]->GetSSTFiles().size() == 1;
        result &= lsmTree->GetLevels()[1]->GetSSTFiles()[0]->GetFileName() == fileName;

        // 3. Clean up
        fs::remove(fileName);
        return result;
    }

    /**
     * Expect the LSM tree returns the most updated <key, value> pair, if exists.
     */
    static bool TestGetWithAllUniqueKeys() {
        LSMTree *lsmTree = Setup();
        if (!lsmTree) {
            return false;
        }

        // 1. Set up data by writing sst files to level 0 and 1
        std::vector<DataEntry_t> data1;
        GetData(0, 512, 10, data1, 1);
        WriteDataToLSMTree(lsmTree, data1);

        std::vector<DataEntry_t> data2;
        GetData(512, 2 * 512 + 1, 10, data2, 1);
        WriteDataToLSMTree(lsmTree, data2);
        lsmTree->MaintainLevelCapacityAndCompact(lsmTree->GetLevels()[0], kvsDirPath);

        std::vector<DataEntry_t> data3;
        GetData(2 * 512 + 1, 3 * 512 + 1, 10, data3, 1);
        WriteDataToLSMTree(lsmTree, data3);

        // 2. Run and check expected values
        bool result = true;

        uint64_t v1 = lsmTree->Get(256);
        uint64_t v2 = lsmTree->Get(2557);
        uint64_t v3 = lsmTree->Get(262400);

        // Test keys that do not exist
        uint64_t v4 = lsmTree->Get(13133000000);
        uint64_t v5 = lsmTree->Get(13133888888);
        result &= lsmTree->GetLevels().size() == 2;
        result &= lsmTree->GetLevels()[0]->GetSSTFiles().size() == 1;
        result &= lsmTree->GetLevels()[0]->GetSSTFiles()[0]->GetFileName() == kvsDirPath + "/level0-0.sst";
        result &= lsmTree->GetLevels()[1]->GetSSTFiles().size() == 1;
        result &= lsmTree->GetLevels()[1]->GetSSTFiles()[0]->GetFileName() == kvsDirPath + "/level1-0.sst";
        result &= v1 == 2560 && v2 == 25570 && v3 == 2624000;
        result &= v4 == std::numeric_limits<uint64_t>::max();
        result &= v5 == std::numeric_limits<uint64_t>::max();  // Not found keys

        // 3. Clean up
        fs::remove_all(kvsDirPath);
        return result;
    }

    /**
     * Expect the LSM tree to scan and return the most updated range of keys requested.
     */
    static bool TestScanWithAllUniqueKeys() {
        LSMTree *lsmTree = Setup();
        if (!lsmTree) {
            return false;
        }

        // 1. Set up data by writing sst files to level 0 and 1
        uint64_t expectedValuesSum = 0;
        std::vector<DataEntry_t> data1;
        expectedValuesSum += GetData(0, 1024, 10, data1, 2);
        WriteDataToLSMTree(lsmTree, data1);

        std::vector<DataEntry_t> data2;
        expectedValuesSum += GetData(1024, (3 * 1024) + 1, 10, data2, 4);
        WriteDataToLSMTree(lsmTree, data2);
        lsmTree->MaintainLevelCapacityAndCompact(lsmTree->GetLevels()[0], kvsDirPath);

        std::vector<DataEntry_t> data3;
        expectedValuesSum += GetData((3 * 1024) + 1, (5 * 1024) + 1, 10, data3, 4);
        WriteDataToLSMTree(lsmTree, data3);

        // 2. Run and check expected values
        bool result = true;

        // The ranges of keys in the LSM tree will be from 0 to 5 * 1024 * 256.
        uint64_t valuesSumInScan = 0;
        std::vector<DataEntry_t> nodesList;
        lsmTree->Scan(3, (5 * 1024 * 256) + 100, nodesList);
        for (DataEntry_t pair : nodesList) {
            valuesSumInScan += pair.second;
        }
        result &= lsmTree->GetLevels().size() == 2;
        result &= lsmTree->GetLevels()[0]->GetSSTFiles().size() == 1;
        result &= lsmTree->GetLevels()[0]->GetSSTFiles()[0]->GetFileName() == kvsDirPath + "/level0-0.sst";
        result &= lsmTree->GetLevels()[1]->GetSSTFiles().size() == 1;
        result &= lsmTree->GetLevels()[1]->GetSSTFiles()[0]->GetFileName() == kvsDirPath + "/level1-0.sst";

        // We will not have the keys equal to 1 and 2, which will have values 10 and 20.
        result &= ((expectedValuesSum - (10 + 20)) == valuesSumInScan);

        // 3. Clean up
        fs::remove_all(kvsDirPath);
        return result;
    }

    static bool TestScanAndGetWithUpdatedAndDeletedKeys() {
        LSMTree *lsmTree = Setup();
        if (!lsmTree) {
            return false;
        }

        uint64_t expectedValuesSum = 0;
        uint64_t defaultValue = 10;
        uint64_t keysToDeleteStart = 0;
        uint64_t keysToDeleteEnd = 256;
        uint64_t keysToUpdateStart = 256;
        uint64_t keysToUpdateEnd = 512;
        uint64_t updatedValue = 20;

        // 1. Set up data by writing sst files to level 0 and 1
        std::vector<DataEntry_t> data1;
        GetData(keysToDeleteStart, keysToDeleteEnd, defaultValue, data1, 1);
        WriteDataToLSMTree(lsmTree, data1);

        std::vector<DataEntry_t> data2;
        expectedValuesSum += GetData(512, (2 * 512) + 1, defaultValue, data2, 1);
        WriteDataToLSMTree(lsmTree, data2);
        lsmTree->MaintainLevelCapacityAndCompact(lsmTree->GetLevels()[0], kvsDirPath);

        std::vector<DataEntry_t> data3;
        // Write some deleted keys
        GetData(keysToDeleteStart, keysToDeleteEnd, Utils::DELETED_KEY_VALUE, data3, 1);
        // Write some updated keys
        expectedValuesSum += GetData(keysToUpdateStart, keysToUpdateEnd, updatedValue, data3, 1);
        WriteDataToLSMTree(lsmTree, data3);

        // 2. Run and check expected values
        bool result = true;

        for (uint64_t i = keysToDeleteStart; i < keysToDeleteEnd; i++) {
            for (uint64_t t = i * 256; t < (i + 1) * 256; t++) {
                result &= lsmTree->Get(t) == std::numeric_limits<uint64_t>::max();
            }
        }
        for (uint64_t i = keysToUpdateStart; i < keysToUpdateEnd; i++) {
            for (uint64_t t = i * 256; t < (i + 1) * 256; t++) {
                result &= lsmTree->Get(t) == t * updatedValue;
            }
        }
        // Test the key that has never existed.
        uint64_t valuesSumInScan = 0;
        std::vector<DataEntry_t> nodesList;
        lsmTree->Scan(0, 263000, nodesList);  // existing keys are 0 to 262144
        for (DataEntry_t pair : nodesList) {
            valuesSumInScan += pair.second;
        }
        result &= lsmTree->GetLevels().size() == 2;
        result &= lsmTree->GetLevels()[0]->GetSSTFiles().size() == 1;
        result &= lsmTree->GetLevels()[0]->GetSSTFiles()[0]->GetFileName() == kvsDirPath + "/level0-0.sst";
        result &= lsmTree->GetLevels()[1]->GetSSTFiles().size() == 1;
        result &= lsmTree->GetLevels()[1]->GetSSTFiles()[0]->GetFileName() == kvsDirPath + "/level1-0.sst";
        result &= expectedValuesSum == valuesSumInScan;

        // 3. Clean up
        fs::remove_all(kvsDirPath);
        return result;
    }

   public:
    bool RunTests() override {
        bool allTestPassed = true;
        allTestPassed &= assertTrue(TestWriteMemtableData, "TestLSMTree::TestWriteMemtableData");
        allTestPassed &=
            assertTrue(TestMaintainLevelsCapacityAndCompact, "TestLSMTree::TestMaintainLevelsCapacityAndCompact");
        allTestPassed &= assertTrue(TestGetWithAllUniqueKeys, "TestLSMTree::TestGetWithAllUniqueKeys");
        allTestPassed &= assertTrue(TestScanWithAllUniqueKeys, "TestLSMTree::TestScanWithAllUniqueKeys");
        allTestPassed &=
            assertTrue(TestScanAndGetWithUpdatedAndDeletedKeys, "TestLSMTree::TestScanAndGetWithUpdatedAndDeletedKeys");
        return allTestPassed;
    }
};
