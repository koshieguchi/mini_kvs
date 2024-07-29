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
    static const SearchType search_type = SearchType::B_TREE_SEARCH;
    static const int bloom_filter_bits_per_entry = 10;
    static const int input_buffer_capacity = 8;
    static const int output_buffer_capacity = 256;
    inline static std::string kvsDirPath = "./kvs_lsm_tree";

    static LSMTree *Setup() {
        if (!fs::exists(kvsDirPath)) {
            bool create_dir_res = fs::create_directories(kvsDirPath);
            if (!create_dir_res) {
                return nullptr;
            }
        }
        return new LSMTree(bloom_filter_bits_per_entry, input_buffer_capacity, output_buffer_capacity);
    }

    static void WriteDataToLSMTree(LSMTree *lsm_tree, std::vector<DataEntry_t> &data) {
        if (lsm_tree->GetLevels().empty()) {
            auto *first_level =
                new Level(0, bloom_filter_bits_per_entry, input_buffer_capacity, output_buffer_capacity);
            lsm_tree->AddLevel(first_level);
        }
        lsm_tree->GetLevels()[0]->WriteDataToLevel(data, search_type, kvsDirPath);
    }

    /**
     * Expect the memtable data to be written to the first lavel of LSM tree.
     */
    static bool TestWriteMemtableData() {
        LSMTree *lsm_tree = Setup();
        if (!lsm_tree) {
            return false;
        }

        // 1. Set up data
        std::vector<DataEntry_t> data;
        GetData(0, 512, 10, data, 1);

        // 2. Run and check expected values
        bool result = true;

        lsm_tree->WriteMemtableData(data, search_type, kvsDirPath);
        std::string file_name = kvsDirPath + "/level0-0.sst";
        result &= lsm_tree->GetLevels().size() == 1;
        result &= lsm_tree->GetLevels()[0]->GetSSTFiles().size() == 1;
        result &= lsm_tree->GetLevels()[0]->GetSSTFiles()[0]->GetFileName() == file_name;

        // 3. Clean up
        fs::remove_all(kvsDirPath);
        return result;
    }

    /**
     * Expect the levels to sort and flush to next level when there are two sst files in them.
     */
    static bool TestMaintainLevelsCapacityAndCompact() {
        LSMTree *lsm_tree = Setup();
        if (!lsm_tree) {
            return false;
        }

        // 1. Set up data by writing 2 sst files to first level
        std::vector<DataEntry_t> data1;
        GetData(0, 512, 10, data1, 1);
        WriteDataToLSMTree(lsm_tree, data1);

        std::vector<DataEntry_t> data2;
        GetData(512, 2 * 512 + 1, 10, data2, 1);
        WriteDataToLSMTree(lsm_tree, data2);

        // 2. Run and check expected values
        bool result = true;

        lsm_tree->MaintainLevelCapacityAndCompact(lsm_tree->GetLevels()[0], kvsDirPath);
        std::string file_name = kvsDirPath + "/level1-0.sst";
        result &= lsm_tree->GetLevels().size() == 2;
        result &= lsm_tree->GetLevels()[0]->GetSSTFiles().empty();
        result &= lsm_tree->GetLevels()[1]->GetSSTFiles().size() == 1;
        result &= lsm_tree->GetLevels()[1]->GetSSTFiles()[0]->GetFileName() == file_name;

        // 3. Clean up
        fs::remove(file_name);
        return result;
    }

    /**
     * Expect the LSM tree returns the most updated <key, value> pair, if exists.
     */
    static bool TestGetWithAllUniqueKeys() {
        LSMTree *lsm_tree = Setup();
        if (!lsm_tree) {
            return false;
        }

        // 1. Set up data by writing sst files to level 0 and 1
        std::vector<DataEntry_t> data1;
        GetData(0, 512, 10, data1, 1);
        WriteDataToLSMTree(lsm_tree, data1);

        std::vector<DataEntry_t> data2;
        GetData(512, 2 * 512 + 1, 10, data2, 1);
        WriteDataToLSMTree(lsm_tree, data2);
        lsm_tree->MaintainLevelCapacityAndCompact(lsm_tree->GetLevels()[0], kvsDirPath);

        std::vector<DataEntry_t> data3;
        GetData(2 * 512 + 1, 3 * 512 + 1, 10, data3, 1);
        WriteDataToLSMTree(lsm_tree, data3);

        // 2. Run and check expected values
        bool result = true;

        uint64_t v1 = lsm_tree->Get(256);
        uint64_t v2 = lsm_tree->Get(2557);
        uint64_t v3 = lsm_tree->Get(262400);

        // Test keys that do not exist
        uint64_t v4 = lsm_tree->Get(13133000000);
        uint64_t v5 = lsm_tree->Get(13133888888);
        result &= lsm_tree->GetLevels().size() == 2;
        result &= lsm_tree->GetLevels()[0]->GetSSTFiles().size() == 1;
        result &= lsm_tree->GetLevels()[0]->GetSSTFiles()[0]->GetFileName() == kvsDirPath + "/level0-0.sst";
        result &= lsm_tree->GetLevels()[1]->GetSSTFiles().size() == 1;
        result &= lsm_tree->GetLevels()[1]->GetSSTFiles()[0]->GetFileName() == kvsDirPath + "/level1-0.sst";
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
        LSMTree *lsm_tree = Setup();
        if (!lsm_tree) {
            return false;
        }

        // 1. Set up data by writing sst files to level 0 and 1
        uint64_t expected_values_sum = 0;
        std::vector<DataEntry_t> data1;
        expected_values_sum += GetData(0, 1024, 10, data1, 2);
        WriteDataToLSMTree(lsm_tree, data1);

        std::vector<DataEntry_t> data2;
        expected_values_sum += GetData(1024, (3 * 1024) + 1, 10, data2, 4);
        WriteDataToLSMTree(lsm_tree, data2);
        lsm_tree->MaintainLevelCapacityAndCompact(lsm_tree->GetLevels()[0], kvsDirPath);

        std::vector<DataEntry_t> data3;
        expected_values_sum += GetData((3 * 1024) + 1, (5 * 1024) + 1, 10, data3, 4);
        WriteDataToLSMTree(lsm_tree, data3);

        // 2. Run and check expected values
        bool result = true;

        // The ranges of keys in the LSM tree will be from 0 to 5 * 1024 * 256.
        uint64_t values_sum_in_scan = 0;
        std::vector<DataEntry_t> nodes_list;
        lsm_tree->Scan(3, (5 * 1024 * 256) + 100, nodes_list);
        for (DataEntry_t pair : nodes_list) {
            values_sum_in_scan += pair.second;
        }
        result &= lsm_tree->GetLevels().size() == 2;
        result &= lsm_tree->GetLevels()[0]->GetSSTFiles().size() == 1;
        result &= lsm_tree->GetLevels()[0]->GetSSTFiles()[0]->GetFileName() == kvsDirPath + "/level0-0.sst";
        result &= lsm_tree->GetLevels()[1]->GetSSTFiles().size() == 1;
        result &= lsm_tree->GetLevels()[1]->GetSSTFiles()[0]->GetFileName() == kvsDirPath + "/level1-0.sst";

        // We will not have the keys equal to 1 and 2, which will have values 10 and 20.
        result &= ((expected_values_sum - (10 + 20)) == values_sum_in_scan);

        // 3. Clean up
        fs::remove_all(kvsDirPath);
        return result;
    }

    static bool TestScanAndGetWithUpdatedAndDeletedKeys() {
        LSMTree *lsm_tree = Setup();
        if (!lsm_tree) {
            return false;
        }

        uint64_t expected_values_sum = 0;
        uint64_t defaultValue = 10;
        uint64_t keysToDeleteStart = 0;
        uint64_t keysToDeleteEnd = 256;
        uint64_t keysToUpdateStart = 256;
        uint64_t keysToUpdateEnd = 512;
        uint64_t updatedValue = 20;

        // 1. Set up data by writing sst files to level 0 and 1
        std::vector<DataEntry_t> data1;
        GetData(keysToDeleteStart, keysToDeleteEnd, defaultValue, data1, 1);
        WriteDataToLSMTree(lsm_tree, data1);

        std::vector<DataEntry_t> data2;
        expected_values_sum += GetData(512, (2 * 512) + 1, defaultValue, data2, 1);
        WriteDataToLSMTree(lsm_tree, data2);
        lsm_tree->MaintainLevelCapacityAndCompact(lsm_tree->GetLevels()[0], kvsDirPath);

        std::vector<DataEntry_t> data3;
        // Write some deleted keys
        GetData(keysToDeleteStart, keysToDeleteEnd, Utils::DELETED_KEY_VALUE, data3, 1);
        // Write some updated keys
        expected_values_sum += GetData(keysToUpdateStart, keysToUpdateEnd, updatedValue, data3, 1);
        WriteDataToLSMTree(lsm_tree, data3);

        // 2. Run and check expected values
        bool result = true;

        for (uint64_t i = keysToDeleteStart; i < keysToDeleteEnd; i++) {
            for (uint64_t t = i * 256; t < (i + 1) * 256; t++) {
                result &= lsm_tree->Get(t) == std::numeric_limits<uint64_t>::max();
            }
        }
        for (uint64_t i = keysToUpdateStart; i < keysToUpdateEnd; i++) {
            for (uint64_t t = i * 256; t < (i + 1) * 256; t++) {
                result &= lsm_tree->Get(t) == t * updatedValue;
            }
        }
        // Test the key that has never existed.
        uint64_t values_sum_in_scan = 0;
        std::vector<DataEntry_t> nodes_list;
        lsm_tree->Scan(0, 263000, nodes_list);  // existing keys are 0 to 262144
        for (DataEntry_t pair : nodes_list) {
            values_sum_in_scan += pair.second;
        }
        result &= lsm_tree->GetLevels().size() == 2;
        result &= lsm_tree->GetLevels()[0]->GetSSTFiles().size() == 1;
        result &= lsm_tree->GetLevels()[0]->GetSSTFiles()[0]->GetFileName() == kvsDirPath + "/level0-0.sst";
        result &= lsm_tree->GetLevels()[1]->GetSSTFiles().size() == 1;
        result &= lsm_tree->GetLevels()[1]->GetSSTFiles()[0]->GetFileName() == kvsDirPath + "/level1-0.sst";
        result &= expected_values_sum == values_sum_in_scan;

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
