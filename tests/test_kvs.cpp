#include <algorithm>
#include <cmath>
#include <filesystem>

#include "kvs.h"
#include "test_base.h"

int buffer_pool_min_size = pow(2, 2);
int buffer_pool_max_size = pow(2, 3);
EvictionPolicyType evictionPolicy = LRU_t;

class TestKVS : public TestBase {
    static bool TestOpen() {
        int memtable_size = 10;
        auto buffer_pool = new BufferPool(buffer_pool_min_size, buffer_pool_max_size, evictionPolicy);
        auto kvs = new KVS(memtable_size, SearchType::BINARY_SEARCH, buffer_pool);
        kvs->Open("test_dir");

        bool result = std::filesystem::is_directory("./test_dir");

        // Clean up
        std::filesystem::remove_all("./test_dir");
        return result;
    }

    static bool TestPut() {
        int memtable_size = 2;
        auto buffer_pool = new BufferPool(buffer_pool_min_size, buffer_pool_max_size, evictionPolicy);
        auto kvs = new KVS(memtable_size, SearchType::BINARY_SEARCH, buffer_pool);
        kvs->Open("test_dir");
        kvs->Put(1, 2);
        kvs->Put(2, 3);  // Should be full after the second Put but haven't written yet
        bool result = std::filesystem::is_empty("./test_dir");

        kvs->Put(3, 4);
        result &= !std::filesystem::is_empty("./test_dir");  // shouldn't be empty after inserting beyond capacity

        // Clean up
        std::filesystem::remove_all("./test_dir");
        return result;
    }

    static bool TestClose() {
        int memtable_size = 10;
        auto buffer_pool = new BufferPool(buffer_pool_min_size, buffer_pool_max_size, evictionPolicy);
        auto kvs = new KVS(memtable_size, SearchType::BINARY_SEARCH, buffer_pool);
        kvs->Open("test_dir");
        kvs->Close();
        bool result = std::filesystem::is_empty("./test_dir");  // empty memtable shouldn't write any SST file

        kvs->Open("test_dir");
        kvs->Put(1, 2);
        kvs->Close();
        result &= !std::filesystem::is_empty("./test_dir");

        // Clean up
        std::filesystem::remove_all("./test_dir");
        return result;
    }

    static bool TestGetBinarySearch() {
        int memtable_size = 1;
        auto buffer_pool = new BufferPool(buffer_pool_min_size, buffer_pool_max_size, evictionPolicy);
        auto kvs = new KVS(memtable_size, SearchType::BINARY_SEARCH, buffer_pool);
        kvs->Open("test_dir");
        kvs->Put(1, 2);
        bool result = kvs->Get(1) == 2;  // Check if kvs access memtable correctly

        kvs->Put(2, 3);
        result &= !std::filesystem::is_empty("./test_dir");  // kvs dir shouldn't be empty after second insert
        result &= kvs->Get(2) == 3;                          // Check if kvs access SST files correctly

        // Clean up
        std::filesystem::remove_all("./test_dir");
        return result;
    }

    static bool TestScanBinarySearch() {
        int memtable_size = 5;
        auto buffer_pool = new BufferPool(buffer_pool_min_size, buffer_pool_max_size, evictionPolicy);
        auto kvs = new KVS(memtable_size, SearchType::BINARY_SEARCH, buffer_pool);
        kvs->Open("test_dir");

        // TODO: implement proper test logic once scan is done
        bool result = true;

        // Clean up
        std::filesystem::remove_all("./test_dir");
        return result;
    }

    static bool TestGetBTreeSearch(KVS *kvs) {
        bool result = true;
        for (uint64_t i = 0; i <= 256; i++) {
            for (uint64_t t = i * 256; t < (i + 1) * 256; t += 2) {
                result &= kvs->Get(t + 1) == (t + 1) * 10;
                result &= kvs->Get(t) == t * 10;
            }
        }
        return result;
    }

    static bool TestScanBTreeSearch(KVS *kvs, uint64_t key1, uint64_t key2, uint64_t expected_values_sum) {
        std::vector<std::pair<uint64_t, uint64_t>> nodes_list;
        kvs->Scan(key1, key2, nodes_list);

        uint64_t keysSum = 0;
        uint64_t valuesSum = 0;
        for (std::pair<uint64_t, uint64_t> pair : nodes_list) {
            keysSum += pair.first;
            valuesSum += pair.second;
        }
        bool result = true;
        result &= (keysSum == expected_values_sum / 10);
        result &= (valuesSum == expected_values_sum);
        return result;
    }

    static bool TestKVSWithBTreeSearch() {
        // Memtable with size 512 pages
        int memtableMaxSizeLimit = (256 * 4096) / 16;
        auto buffer_pool = new BufferPool(buffer_pool_min_size, buffer_pool_max_size, evictionPolicy);
        auto kvs = new KVS(memtableMaxSizeLimit, SearchType::B_TREE_SEARCH, buffer_pool);
        kvs->Open("test_dir");
        // Write 512 pages of key-value pairs and one extra page
        // to force the memtable to write its content to an sst file.
        uint64_t expected_values_sum = 0;
        for (uint64_t i = 0; i <= 256; i++) {
            for (uint64_t t = i * 256; t < (i + 1) * 256; t++) {
                kvs->Put(t, t * 10);
                expected_values_sum += t * 10;
            }
        }

        bool result = true;
        result &= TestGetBTreeSearch(kvs);
        result &= TestScanBTreeSearch(kvs, 0, (256 + 1) * 256 + 100, expected_values_sum);

        // Clean up
        std::filesystem::remove_all("./test_dir");
        return result;
    }

   public:
    bool RunTests() override {
        bool result = true;
        result &= assertTrue(TestOpen, "TestKVS::TestOpen");
        result &= assertTrue(TestPut, "TestKVS::TestPut");
        result &= assertTrue(TestClose, "TestKVS::TestClose");
        result &= assertTrue(TestGetBinarySearch, "TestKVS::TestGetBinarySearch");
        result &= assertTrue(TestScanBinarySearch, "TestKVS::TestScanBinarySearch");
        result &= assertTrue(TestKVSWithBTreeSearch, "TestKVS::TestKVSWithBTreeSearch");
        return result;
    }
};
