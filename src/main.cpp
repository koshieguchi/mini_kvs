#include <iostream>

#include "db.h"

/* These are some examples of how a user should create and call the database. */

void runTestOnDbWithBinarySearch() {
    // Memtable with size 512 pages
    int memtableNumOfElements = (512 * 4096) / 16;
    int minBufferPoolSize = 5;
    int maxBufferPoolSize = 16;
    auto bufferPool = new BufferPool(minBufferPoolSize, maxBufferPoolSize, EvictionPolicyType::LRU_t);
    Db *db = new Db(memtableNumOfElements, SearchType::BINARY_SEARCH, bufferPool);
    db->Open("./my_db");

    // Test Put
    // Write 512 pages of key-value pairs and one extra page
    // to force the memtable to write its content to an sst file.
    uint64_t expectedValuesSum = 0;
    for (uint64_t i = 1; i <= 513; i++) {
        for (uint64_t t = i * 256; t < (i + 1) * 256; t++) {
            db->Put(t, t * 10);
            expectedValuesSum += t * 10;
        }
    }

    // Test Get
    uint64_t v1 = db->Get(256);
    std::cout << "v1 is: " << v1 << " " << (v1 == 2560) << std::endl;

    uint64_t v2 = db->Get(131327);
    std::cout << "v2 is: " << v2 << " " << (v2 == 1313270) << std::endl;

    // Test Get on all
    uint64_t valuesSumInGet = 0;
    for (uint64_t i = 1; i <= 513; i++) {
        for (uint64_t t = i * 256; t < (i + 1) * 256; t++) {
            uint64_t value = db->Get(t);
            valuesSumInGet += value;
        }
    }
    if (valuesSumInGet == expectedValuesSum) {
        std::cout << "Tests Succeeded with binary search in get :))))!" << std::endl;
    } else {
        std::cout << "Tests Failed in get! " << " expectedValuesSum: " << expectedValuesSum
                  << " while valuesSumInGet: " << valuesSumInGet << std::endl;
    }

    std::vector<DataEntry_t> nodesList;
    // The ranges of keys inside the file is 0 <= ... <= 131583, but we test it on ranges beyond.
    db->Scan(0, 131583 + 10, nodesList);

    uint64_t valuesSumInScan = 0;
    for (DataEntry_t pair : nodesList) {
        valuesSumInScan += pair.second;
    }

    if (valuesSumInScan == expectedValuesSum) {
        std::cout << "Tests Succeeded with binary search in scan :))))!" << std::endl;
    } else {
        std::cout << "Tests Failed in binary scan! " << " expectedValuesSum: " << expectedValuesSum
                  << " while valuesSumInScan: " << valuesSumInScan << std::endl;
    }

    db->Close();
}

void runTestOnDbWithIncompleteBTree() {
    // Memtable with size 510 pages
    int memtableNumOfElements = (510 * 4096) / 16;
    int minBufferPoolSize = 5;
    int maxBufferPoolSize = 16;
    auto bufferPool = new BufferPool(minBufferPoolSize, maxBufferPoolSize, EvictionPolicyType::LRU_t);
    Db *db = new Db(memtableNumOfElements, SearchType::B_TREE_SEARCH, bufferPool);
    db->Open("./my_db_incomplete_BTree");
    // Write 512 pages of key-value pairs and one extra page
    // to force the memtable to write its content to an sst file.
    uint64_t expectedValuesSum = 0;
    uint64_t expectedKeysSum = 0;
    for (int i = 0; i < 512; i++) {
        for (uint64_t t = i * 256; t < (i + 1) * 256; t++) {
            db->Put(t, t * 10);
            expectedKeysSum += t;
            expectedValuesSum += t * 10;
        }
    }

    uint64_t v1 = db->Get(256);
    std::cout << "v1 is: " << v1 << " and " << (v1 == 2560) << std::endl;  // expected == 2560

    uint64_t v2 = db->Get(130559);
    std::cout << "v2 is: " << v2 << " and " << (v2 == 1305590) << std::endl;  // expected == 1305590

    // Test Get on all
    uint64_t valuesSumInGet = 0;
    for (uint64_t i = 0; i < 512; i++) {
        for (uint64_t t = i * 256; t < (i + 1) * 256; t++) {
            uint64_t value = db->Get(t);
            valuesSumInGet += value;
        }
    }
    if (valuesSumInGet == expectedValuesSum) {
        std::cout << "Tests Succeeded :))))!" << std::endl;
    } else {
        std::cout << "Tests Failed! " << " expectedValuesSum: " << expectedValuesSum
                  << " while valuesSumInGet: " << valuesSumInGet << std::endl;
    }

    // Test Scan
    uint64_t keysSumInScan = 0;
    uint64_t valuesSumInScan = 0;
    std::vector<DataEntry_t> nodesList;
    db->Scan(270, 512 * 256, nodesList);
    expectedKeysSum = expectedKeysSum - (269 * 270 / 2);
    for (DataEntry_t pair : nodesList) {
        keysSumInScan += pair.first;
        valuesSumInScan += pair.second;
    }

    if (keysSumInScan == expectedKeysSum) {
        std::cout << "Tests Succeeded in scan :))))!" << std::endl;
    } else {
        std::cout << "Tests Failed in scan! " << " expectedKeysSum: " << expectedKeysSum
                  << " while keysSumInScan: " << keysSumInScan << std::endl;
    }
    db->Close();
    // std::filesystem::remove_all("my_db_incomplete_BTree");
}

void runTestOnDbWithCompleteBTree() {
    // Memtable with size 512 pages
    int memtableNumOfElements = (512 * 4096) / 16;
    int minBufferPoolSize = 5;
    int maxBufferPoolSize = 16;
    auto bufferPool = new BufferPool(minBufferPoolSize, maxBufferPoolSize, EvictionPolicyType::CLOCK_t);
    Db *db = new Db(memtableNumOfElements, SearchType::B_TREE_SEARCH, bufferPool);
    db->Open("./my_db");

    // Test Put
    // Write 512 pages of key-value pairs and one extra page
    // to force the memtable to write its content to an sst file.
    uint64_t expectedValuesSum = 0;
    for (uint64_t i = 0; i <= 512; i++) {
        for (uint64_t t = i * 256; t < (i + 1) * 256; t++) {
            db->Put(t, t * 10);
            expectedValuesSum += t * 10;
        }
    }

    // Test Get
    uint64_t v1 = db->Get(256);
    std::cout << "v1 is: " << v1 << " " << (v1 == 2560) << std::endl;  // expected == 2560

    uint64_t v2 = db->Get(2557);
    std::cout << "v2 is: " << v2 << " " << (v2 == 25570) << std::endl;  // expected == 25570

    // Test Get on all
    uint64_t valuesSumInGet = 0;
    for (uint64_t i = 0; i <= 512; i++) {
        for (uint64_t t = i * 256; t < (i + 1) * 256; t++) {
            uint64_t value = db->Get(t);
            valuesSumInGet += value;
        }
    }
    if (valuesSumInGet == expectedValuesSum) {
        std::cout << "Tests Succeeded in Btree get :))))!" << std::endl;
    } else {
        std::cout << "Tests Failed in get! " << " expectedValuesSum: " << expectedValuesSum
                  << " while valuesSumInGet: " << valuesSumInGet << std::endl;
    }

    // Test Scan
    uint64_t valuesSumInScan = 0;
    std::vector<DataEntry_t> nodesList;
    db->Scan(0, 131329, nodesList);  // There are 513 pages of length 256. So we have from 0 to 131327.
    for (DataEntry_t pair : nodesList) {
        valuesSumInScan += pair.second;
    }
    if (valuesSumInScan == expectedValuesSum) {
        std::cout << "Tests Succeeded in Btree scan :))))!" << std::endl;
    } else {
        std::cout << "Tests Failed in scan! " << " expectedValuesSum: " << expectedValuesSum
                  << " while valuesSumInScan: " << valuesSumInScan << std::endl;
    }
    db->Close();
}

void runTestOnDbWithBTreeMediumFile() {
    // Btree with < 0.5GB file
    // Memtable will have 512 * 256 pages
    int memtableNumOfElements = ((511 * 256) * 4096) / 16;
    int minBufferPoolSize = 5;
    int maxBufferPoolSize = 16;
    auto bufferPool = new BufferPool(minBufferPoolSize, maxBufferPoolSize, EvictionPolicyType::LRU_t);
    Db *db = new Db(memtableNumOfElements, SearchType::B_TREE_SEARCH, bufferPool);
    db->Open("./my_db");
    // Write 512 * 256 pages of key-value pairs and one extra page
    // to force the memtable to write its content to an sst file.
    uint64_t expectedValuesSum = 0;
    for (uint64_t i = 0; i < 512 * 256; i++) {
        for (uint64_t t = i * 256; t < (i + 1) * 256; t++) {
            db->Put(t, t * 10);
            expectedValuesSum += t * 10;
        }
    }

    uint64_t v1 = db->Get(256);
    std::cout << "v1 is: " << v1 << std::endl;  // == 2560

    uint64_t v2 = db->Get(131072);
    std::cout << "v2 is: " << v2 << std::endl;  // == 1310720

    uint64_t valuesSum = 0;
    std::vector<DataEntry_t> nodesList;
    db->Scan(0, 512 * 256 * 256, nodesList);
    for (DataEntry_t pair : nodesList) {
        // std::cout << "(" << pair.first << "," << pair.second << ")" << std::endl;
        valuesSum += pair.second;
    }

    if (valuesSum == expectedValuesSum) {
        std::cout << "Tests Succeeded :))))!" << std::endl;
    } else {
        std::cout << "Tests Failed! " << " expectedValuesSum: " << expectedValuesSum
                  << " while valuesSum: " << valuesSum << std::endl;
    }
}

void runTestOnDbWithBTreeBigFile() {
    // Btree with ~ 1GB file
    // Memtable will have 512 * 512 pages
    int memtableNumOfElements = ((512 * 512) * 4096) / 16;
    int minBufferPoolSize = 5;
    int maxBufferPoolSize = 16;
    auto bufferPool = new BufferPool(minBufferPoolSize, maxBufferPoolSize, EvictionPolicyType::LRU_t);
    Db *db = new Db(memtableNumOfElements, SearchType::B_TREE_SEARCH, bufferPool);
    db->Open("./my_db");
    // Write 512 * 512 pages of key-value pairs and one extra page
    // to force the memtable to write its content to an sst file.
    uint64_t expectedValuesSum = 0;
    for (uint64_t i = 0; i <= 512 * 512; i++) {
        for (uint64_t t = i * 256; t < (i + 1) * 256; t++) {
            db->Put(t, t * 10);
            expectedValuesSum += t * 10;
        }
    }

    uint64_t v1 = db->Get(256);
    std::cout << "v1 is: " << v1 << std::endl;  // == 2560

    uint64_t v2 = db->Get(131071 + 512);
    std::cout << "v2 is: " << v2 << std::endl;  // == 1315830

    // Test Get on all
    uint64_t valuesSumInGet = 0;
    for (uint64_t i = 0; i <= 512 * 512; i++) {
        for (uint64_t t = i * 256; t < (i + 1) * 256; t++) {
            uint64_t value = db->Get(t);
            valuesSumInGet += value;
        }
    }
    if (valuesSumInGet == expectedValuesSum) {
        std::cout << "Tests Succeeded in get :))))!" << std::endl;
    } else {
        std::cout << "Tests Failed in get! " << " expectedValuesSum: " << expectedValuesSum
                  << " while valuesSumInGet: " << valuesSumInGet << std::endl;
    }
}

void runLSMTreeGet(Db *db, uint64_t expectedValuesSum) {
    // Test Get
    uint64_t v1 = db->Get(256);
    std::cout << "v1 is: " << v1 << " " << (v1 == 2560) << std::endl;  // expected == 2560

    uint64_t v2 = db->Get(2557);
    std::cout << "v2 is: " << v2 << " " << (v2 == 25570) << std::endl;  // expected == 25570

    uint64_t v3 = db->Get(131325);
    std::cout << "v3 is: " << v3 << " " << (v3 == 1313250) << std::endl;  // expected == 1313250

    // Test keys that do not exist
    uint64_t v4 = db->Get(131330);
    std::cout << "v4 is: " << v4 << " " << (v4 == std::numeric_limits<uint64_t>::max()) << " NOT FOUND"
              << std::endl;  // expected == std::numeric_limits<uint64_t>::max()

    uint64_t v5 = db->Get(13133888888);
    std::cout << "v5 is: " << v5 << " " << (v5 == std::numeric_limits<uint64_t>::max()) << " NOT FOUND"
              << std::endl;  // expected == std::numeric_limits<uint64_t>::max()

    // Test Get on all
    uint64_t valuesSumInGet = 0;
    for (uint64_t i = 0; i <= 512; i++) {
        for (uint64_t t = i * 256; t < (i + 1) * 256; t++) {
            uint64_t value = db->Get(t);
            valuesSumInGet += value;
        }
    }
    if (valuesSumInGet == expectedValuesSum) {
        std::cout << "Tests Succeeded in LSM Tree get :))))!" << std::endl;
    } else {
        std::cout << "Tests Failed in LSM Tree get! " << " expectedValuesSum: " << expectedValuesSum
                  << " while valuesSumInGet: " << valuesSumInGet << std::endl;
    }
}

void runLSMTreeScan(Db *db, uint64_t expectedValuesSum) {
    // Test Scan
    uint64_t valuesSumInScan = 0;
    std::vector<DataEntry_t> nodesList;
    db->Scan(0, 131329, nodesList);  // There are 513 pages of length 256. So we have from 0 to 131327.
    for (DataEntry_t pair : nodesList) {
        valuesSumInScan += pair.second;
    }
    if (valuesSumInScan == expectedValuesSum) {
        std::cout << "Tests Succeeded in LSM Tree scan :))))!" << std::endl;
    } else {
        std::cout << "Tests Failed in LSM Tree scan! " << " expectedValuesSum: " << expectedValuesSum
                  << " while valuesSumInScan: " << valuesSumInScan << std::endl;
    }
}

void runTestDbWithLSMTree() {
    // Memtable with size 512 pages
    int memtableNumOfElements = (512 * 4096) / 16;
    int minBufferPoolSize = 5;
    int maxBufferPoolSize = 16;
    int bloomFilterBitsPerEntry = 10;
    int inputBufferNumPagesCapacity = 8;
    int outputBufferNumPagesCapacity = 8;

    auto bufferPool = new BufferPool(minBufferPoolSize, maxBufferPoolSize, EvictionPolicyType::CLOCK_t);
    auto lsmTree = new LSMTree(bloomFilterBitsPerEntry, inputBufferNumPagesCapacity, outputBufferNumPagesCapacity);
    Db *db = new Db(memtableNumOfElements, SearchType::B_TREE_SEARCH, bufferPool, lsmTree);
    db->Open("./my_db_lsm_tree");

    // Test Put
    // Write 512 pages of key-value pairs and one extra page
    // to force the memtable to write its content to an sst file.
    uint64_t expectedValuesSum = 0;
    for (uint64_t i = 0; i <= 512; i++) {
        for (uint64_t t = i * 256; t < (i + 1) * 256; t++) {
            db->Put(t, t * 10);
            expectedValuesSum += t * 10;
        }
    }

    runLSMTreeGet(db, expectedValuesSum);
    runLSMTreeScan(db, expectedValuesSum);

    db->Close();

    db->Open("./my_db_lsm_tree");
    std::cout << "Try again!!!!!!!!!" << std::endl;
    runLSMTreeGet(db, expectedValuesSum);
    runLSMTreeScan(db, expectedValuesSum);

    db->Close();
}

int main(int argc, char *argv[]) {
    runTestOnDbWithBinarySearch();
    runTestOnDbWithCompleteBTree();
    runTestOnDbWithIncompleteBTree();
    runTestDbWithLSMTree();
    return 0;
}
