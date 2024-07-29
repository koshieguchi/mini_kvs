#include <iostream>

#include "kvs.h"

/* These are some examples of how a user should create and call the database. */

void runTestOnKVSWithBinarySearch() {
    // Memtable with size 512 pages
    int memtable_num_of_elements = (512 * 4096) / 16;
    int min_buffer_pool_size = 5;
    int max_buffer_pool_size = 16;
    auto buffer_pool = new BufferPool(min_buffer_pool_size, max_buffer_pool_size, EvictionPolicyType::LRU_t);
    KVS *kvs = new KVS(memtable_num_of_elements, SearchType::BINARY_SEARCH, buffer_pool);
    kvs->Open("./my_kvs");

    // Test Put
    // Write 512 pages of key-value pairs and one extra page
    // to force the memtable to write its content to an sst file.
    uint64_t expected_values_sum = 0;
    for (uint64_t i = 1; i <= 513; i++) {
        for (uint64_t t = i * 256; t < (i + 1) * 256; t++) {
            kvs->Put(t, t * 10);
            expected_values_sum += t * 10;
        }
    }

    // Test Get
    uint64_t v1 = kvs->Get(256);
    std::cout << "v1 is: " << v1 << " " << (v1 == 2560) << std::endl;

    uint64_t v2 = kvs->Get(131327);
    std::cout << "v2 is: " << v2 << " " << (v2 == 1313270) << std::endl;

    // Test Get on all
    uint64_t values_sum_in_get = 0;
    for (uint64_t i = 1; i <= 513; i++) {
        for (uint64_t t = i * 256; t < (i + 1) * 256; t++) {
            uint64_t value = kvs->Get(t);
            values_sum_in_get += value;
        }
    }
    if (values_sum_in_get == expected_values_sum) {
        std::cout << "Tests Succeeded with binary search in get :))))!" << std::endl;
    } else {
        std::cout << "Tests Failed in get! " << " expected_values_sum: " << expected_values_sum
                  << " while values_sum_in_get: " << values_sum_in_get << std::endl;
    }

    std::vector<DataEntry_t> nodes_list;
    // The ranges of keys inside the file is 0 <= ... <= 131583, but we test it on ranges beyond.
    kvs->Scan(0, 131583 + 10, nodes_list);

    uint64_t values_sum_in_scan = 0;
    for (DataEntry_t pair : nodes_list) {
        values_sum_in_scan += pair.second;
    }

    if (values_sum_in_scan == expected_values_sum) {
        std::cout << "Tests Succeeded with binary search in scan :))))!" << std::endl;
    } else {
        std::cout << "Tests Failed in binary scan! " << " expected_values_sum: " << expected_values_sum
                  << " while values_sum_in_scan: " << values_sum_in_scan << std::endl;
    }

    kvs->Close();
}

void runTestOnKVSWithIncompleteBTree() {
    // Memtable with size 510 pages
    int memtable_num_of_elements = (510 * 4096) / 16;
    int min_buffer_pool_size = 5;
    int max_buffer_pool_size = 16;
    auto buffer_pool = new BufferPool(min_buffer_pool_size, max_buffer_pool_size, EvictionPolicyType::LRU_t);
    KVS *kvs = new KVS(memtable_num_of_elements, SearchType::B_TREE_SEARCH, buffer_pool);
    kvs->Open("./my_kvs_incomplete_BTree");
    // Write 512 pages of key-value pairs and one extra page
    // to force the memtable to write its content to an sst file.
    uint64_t expected_values_sum = 0;
    uint64_t expected_keys_sum = 0;
    for (int i = 0; i < 512; i++) {
        for (uint64_t t = i * 256; t < (i + 1) * 256; t++) {
            kvs->Put(t, t * 10);
            expected_keys_sum += t;
            expected_values_sum += t * 10;
        }
    }

    uint64_t v1 = kvs->Get(256);
    std::cout << "v1 is: " << v1 << " and " << (v1 == 2560) << std::endl;  // expected == 2560

    uint64_t v2 = kvs->Get(130559);
    std::cout << "v2 is: " << v2 << " and " << (v2 == 1305590) << std::endl;  // expected == 1305590

    // Test Get on all
    uint64_t values_sum_in_get = 0;
    for (uint64_t i = 0; i < 512; i++) {
        for (uint64_t t = i * 256; t < (i + 1) * 256; t++) {
            uint64_t value = kvs->Get(t);
            values_sum_in_get += value;
        }
    }
    if (values_sum_in_get == expected_values_sum) {
        std::cout << "Tests Succeeded :))))!" << std::endl;
    } else {
        std::cout << "Tests Failed! " << " expected_values_sum: " << expected_values_sum
                  << " while values_sum_in_get: " << values_sum_in_get << std::endl;
    }

    // Test Scan
    uint64_t keys_sum_in_scan = 0;
    // uint64_t values_sum_in_scan = 0; // [*] not used
    std::vector<DataEntry_t> nodes_list;
    kvs->Scan(270, 512 * 256, nodes_list);
    expected_keys_sum = expected_keys_sum - (269 * 270 / 2);
    for (DataEntry_t pair : nodes_list) {
        keys_sum_in_scan += pair.first;
        // values_sum_in_scan += pair.second; // [*] not used
    }

    if (keys_sum_in_scan == expected_keys_sum) {
        std::cout << "Tests Succeeded in scan :))))!" << std::endl;
    } else {
        std::cout << "Tests Failed in scan! " << " expected_keys_sum: " << expected_keys_sum
                  << " while keys_sum_in_scan: " << keys_sum_in_scan << std::endl;
    }
    kvs->Close();
    // std::filesystem::remove_all("my_kvs_incomplete_BTree");
}

void runTestOnKVSWithCompleteBTree() {
    // Memtable with size 512 pages
    int memtable_num_of_elements = (512 * 4096) / 16;
    int min_buffer_pool_size = 5;
    int max_buffer_pool_size = 16;
    auto buffer_pool = new BufferPool(min_buffer_pool_size, max_buffer_pool_size, EvictionPolicyType::CLOCK_t);
    KVS *kvs = new KVS(memtable_num_of_elements, SearchType::B_TREE_SEARCH, buffer_pool);
    kvs->Open("./my_kvs");

    // Test Put
    // Write 512 pages of key-value pairs and one extra page
    // to force the memtable to write its content to an sst file.
    uint64_t expected_values_sum = 0;
    for (uint64_t i = 0; i <= 512; i++) {
        for (uint64_t t = i * 256; t < (i + 1) * 256; t++) {
            kvs->Put(t, t * 10);
            expected_values_sum += t * 10;
        }
    }

    // Test Get
    uint64_t v1 = kvs->Get(256);
    std::cout << "v1 is: " << v1 << " " << (v1 == 2560) << std::endl;  // expected == 2560

    uint64_t v2 = kvs->Get(2557);
    std::cout << "v2 is: " << v2 << " " << (v2 == 25570) << std::endl;  // expected == 25570

    // Test Get on all
    uint64_t values_sum_in_get = 0;
    for (uint64_t i = 0; i <= 512; i++) {
        for (uint64_t t = i * 256; t < (i + 1) * 256; t++) {
            uint64_t value = kvs->Get(t);
            values_sum_in_get += value;
        }
    }
    if (values_sum_in_get == expected_values_sum) {
        std::cout << "Tests Succeeded in Btree get :))))!" << std::endl;
    } else {
        std::cout << "Tests Failed in get! " << " expected_values_sum: " << expected_values_sum
                  << " while values_sum_in_get: " << values_sum_in_get << std::endl;
    }

    // Test Scan
    uint64_t values_sum_in_scan = 0;
    std::vector<DataEntry_t> nodes_list;
    kvs->Scan(0, 131329, nodes_list);  // There are 513 pages of length 256. So we have from 0 to 131327.
    for (DataEntry_t pair : nodes_list) {
        values_sum_in_scan += pair.second;
    }
    if (values_sum_in_scan == expected_values_sum) {
        std::cout << "Tests Succeeded in Btree scan :))))!" << std::endl;
    } else {
        std::cout << "Tests Failed in scan! " << " expected_values_sum: " << expected_values_sum
                  << " while values_sum_in_scan: " << values_sum_in_scan << std::endl;
    }
    kvs->Close();
}

void runTestOnKVSWithBTreeMediumFile() {
    // Btree with < 0.5GB file
    // Memtable will have 512 * 256 pages
    int memtable_num_of_elements = ((511 * 256) * 4096) / 16;
    int min_buffer_pool_size = 5;
    int max_buffer_pool_size = 16;
    auto buffer_pool = new BufferPool(min_buffer_pool_size, max_buffer_pool_size, EvictionPolicyType::LRU_t);
    KVS *kvs = new KVS(memtable_num_of_elements, SearchType::B_TREE_SEARCH, buffer_pool);
    kvs->Open("./my_kvs");
    // Write 512 * 256 pages of key-value pairs and one extra page
    // to force the memtable to write its content to an sst file.
    uint64_t expected_values_sum = 0;
    for (uint64_t i = 0; i < 512 * 256; i++) {
        for (uint64_t t = i * 256; t < (i + 1) * 256; t++) {
            kvs->Put(t, t * 10);
            expected_values_sum += t * 10;
        }
    }

    uint64_t v1 = kvs->Get(256);
    std::cout << "v1 is: " << v1 << std::endl;  // == 2560

    uint64_t v2 = kvs->Get(131072);
    std::cout << "v2 is: " << v2 << std::endl;  // == 1310720

    uint64_t valuesSum = 0;
    std::vector<DataEntry_t> nodes_list;
    kvs->Scan(0, 512 * 256 * 256, nodes_list);
    for (DataEntry_t pair : nodes_list) {
        // std::cout << "(" << pair.first << "," << pair.second << ")" << std::endl;
        valuesSum += pair.second;
    }

    if (valuesSum == expected_values_sum) {
        std::cout << "Tests Succeeded :))))!" << std::endl;
    } else {
        std::cout << "Tests Failed! " << " expected_values_sum: " << expected_values_sum
                  << " while valuesSum: " << valuesSum << std::endl;
    }
}

void runTestOnKVSWithBTreeBigFile() {
    // Btree with ~ 1GB file
    // Memtable will have 512 * 512 pages
    int memtable_num_of_elements = ((512 * 512) * 4096) / 16;
    int min_buffer_pool_size = 5;
    int max_buffer_pool_size = 16;
    auto buffer_pool = new BufferPool(min_buffer_pool_size, max_buffer_pool_size, EvictionPolicyType::LRU_t);
    KVS *kvs = new KVS(memtable_num_of_elements, SearchType::B_TREE_SEARCH, buffer_pool);
    kvs->Open("./my_kvs");
    // Write 512 * 512 pages of key-value pairs and one extra page
    // to force the memtable to write its content to an sst file.
    uint64_t expected_values_sum = 0;
    for (uint64_t i = 0; i <= 512 * 512; i++) {
        for (uint64_t t = i * 256; t < (i + 1) * 256; t++) {
            kvs->Put(t, t * 10);
            expected_values_sum += t * 10;
        }
    }

    uint64_t v1 = kvs->Get(256);
    std::cout << "v1 is: " << v1 << std::endl;  // == 2560

    uint64_t v2 = kvs->Get(131071 + 512);
    std::cout << "v2 is: " << v2 << std::endl;  // == 1315830

    // Test Get on all
    uint64_t values_sum_in_get = 0;
    for (uint64_t i = 0; i <= 512 * 512; i++) {
        for (uint64_t t = i * 256; t < (i + 1) * 256; t++) {
            uint64_t value = kvs->Get(t);
            values_sum_in_get += value;
        }
    }
    if (values_sum_in_get == expected_values_sum) {
        std::cout << "Tests Succeeded in get :))))!" << std::endl;
    } else {
        std::cout << "Tests Failed in get! " << " expected_values_sum: " << expected_values_sum
                  << " while values_sum_in_get: " << values_sum_in_get << std::endl;
    }
}

void runLSMTreeGet(KVS *kvs, uint64_t expected_values_sum) {
    // Test Get
    uint64_t v1 = kvs->Get(256);
    std::cout << "v1 is: " << v1 << " " << (v1 == 2560) << std::endl;  // expected == 2560

    uint64_t v2 = kvs->Get(2557);
    std::cout << "v2 is: " << v2 << " " << (v2 == 25570) << std::endl;  // expected == 25570

    uint64_t v3 = kvs->Get(131325);
    std::cout << "v3 is: " << v3 << " " << (v3 == 1313250) << std::endl;  // expected == 1313250

    // Test keys that do not exist
    uint64_t v4 = kvs->Get(131330);
    std::cout << "v4 is: " << v4 << " " << (v4 == std::numeric_limits<uint64_t>::max()) << " NOT FOUND"
              << std::endl;  // expected == std::numeric_limits<uint64_t>::max()

    uint64_t v5 = kvs->Get(13133888888);
    std::cout << "v5 is: " << v5 << " " << (v5 == std::numeric_limits<uint64_t>::max()) << " NOT FOUND"
              << std::endl;  // expected == std::numeric_limits<uint64_t>::max()

    // Test Get on all
    uint64_t values_sum_in_get = 0;
    for (uint64_t i = 0; i <= 512; i++) {
        for (uint64_t t = i * 256; t < (i + 1) * 256; t++) {
            uint64_t value = kvs->Get(t);
            values_sum_in_get += value;
        }
    }
    if (values_sum_in_get == expected_values_sum) {
        std::cout << "Tests Succeeded in LSM Tree get :))))!" << std::endl;
    } else {
        std::cout << "Tests Failed in LSM Tree get! " << " expected_values_sum: " << expected_values_sum
                  << " while values_sum_in_get: " << values_sum_in_get << std::endl;
    }
}

void runLSMTreeScan(KVS *kvs, uint64_t expected_values_sum) {
    // Test Scan
    uint64_t values_sum_in_scan = 0;
    std::vector<DataEntry_t> nodes_list;
    kvs->Scan(0, 131329, nodes_list);  // There are 513 pages of length 256. So we have from 0 to 131327.
    for (DataEntry_t pair : nodes_list) {
        values_sum_in_scan += pair.second;
    }
    if (values_sum_in_scan == expected_values_sum) {
        std::cout << "Tests Succeeded in LSM Tree scan :))))!" << std::endl;
    } else {
        std::cout << "Tests Failed in LSM Tree scan! " << " expected_values_sum: " << expected_values_sum
                  << " while values_sum_in_scan: " << values_sum_in_scan << std::endl;
    }
}

void runTestKVSWithLSMTree() {
    // Memtable with size 512 pages
    int memtable_num_of_elements = (512 * 4096) / 16;
    int min_buffer_pool_size = 5;
    int max_buffer_pool_size = 16;
    int bloom_filter_bits_per_entry = 10;
    int input_buffer_num_pages_capacity = 8;
    int output_buffer_num_pages_capacity = 8;

    auto buffer_pool = new BufferPool(min_buffer_pool_size, max_buffer_pool_size, EvictionPolicyType::CLOCK_t);
    auto lsm_tree =
        new LSMTree(bloom_filter_bits_per_entry, input_buffer_num_pages_capacity, output_buffer_num_pages_capacity);
    KVS *kvs = new KVS(memtable_num_of_elements, SearchType::B_TREE_SEARCH, buffer_pool, lsm_tree);
    kvs->Open("./my_kvs_lsm_tree");

    // Test Put
    // Write 512 pages of key-value pairs and one extra page
    // to force the memtable to write its content to an sst file.
    uint64_t expected_values_sum = 0;
    for (uint64_t i = 0; i <= 512; i++) {
        for (uint64_t t = i * 256; t < (i + 1) * 256; t++) {
            kvs->Put(t, t * 10);
            expected_values_sum += t * 10;
        }
    }

    runLSMTreeGet(kvs, expected_values_sum);
    runLSMTreeScan(kvs, expected_values_sum);

    kvs->Close();

    kvs->Open("./my_kvs_lsm_tree");
    std::cout << "Try again!!!!!!!!!" << std::endl;
    runLSMTreeGet(kvs, expected_values_sum);
    runLSMTreeScan(kvs, expected_values_sum);

    kvs->Close();
}

int main(int argc, char *argv[]) {
    runTestOnKVSWithBinarySearch();
    runTestOnKVSWithCompleteBTree();
    runTestOnKVSWithIncompleteBTree();
    runTestKVSWithLSMTree();
    return 0;
}
