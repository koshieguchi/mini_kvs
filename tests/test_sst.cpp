#include <iostream>
#include <string>

#include "TestBase.h"
#include "sst.h"

class TestSST : public TestBase {
    static bool TestWriteFile() {
        std::string filename = Utils::GetFilenameWithExt("test");
        std::vector<DataEntry_t> dataToWrite = {std::make_pair(1, 2), std::make_pair(2, 3)};
        SST *sstFile = new SST(filename, dataToWrite.size() * SST::KV_PAIR_BYTE_SIZE);
        std::ofstream file(sstFile->GetFileName(), std::ios::out | std::ios::binary);
        sstFile->WriteFile(file, dataToWrite, SearchType::BINARY_SEARCH, true);

        std::ifstream inputFile(filename);
        bool result = inputFile.good();

        // Clean up
        std::remove(filename.c_str());

        return result;
    }

    static bool TestReadOnePageOfFile() {
        // Setup
        std::string filename = Utils::GetFilenameWithExt("test");
        std::vector<DataEntry_t> dataToWrite = {
            std::make_pair(1, 2), std::make_pair(2, 3), std::make_pair(3, 3),
            std::make_pair(4, 3), std::make_pair(5, 3),
        };
        SST *sstFile = new SST(filename, dataToWrite.size() * SST::KV_PAIR_BYTE_SIZE);
        std::ofstream file(sstFile->GetFileName(), std::ios::out | std::ios::binary);
        sstFile->WriteFile(file, dataToWrite, SearchType::BINARY_SEARCH, true);

        // Perform action
        std::ifstream inputStream(filename, std::ifstream::binary);
        off_t offset = 0;
        int fd = Utils::OpenFile(filename);
        std::vector<uint64_t> data = SST::ReadPagesOfFile(fd, offset);

        // Check results
        uint64_t expectedKeyValues[10] = {1, 2, 2, 3, 3, 3, 4, 3, 5, 3};
        for (int i = 0; i < 5; i++) {
            // Check if expected key exists
            if (expectedKeyValues[i] != data[i]) {
                return false;
            }
        }

        // Clean up
        std::remove(filename.c_str());

        return true;
    }

    static bool TestPerformBinarySearchFullSST() {
        // Setup
        std::string filename = Utils::GetFilenameWithExt("test");
        std::vector<DataEntry_t> dataToWrite;
        for (int i = 0; i < 3 * SST::KV_PAIRS_PER_PAGE; i++) {
            dataToWrite.emplace_back(i, i + 10);
        }
        SST *sstFile = new SST(filename, dataToWrite.size() * SST::KV_PAIR_BYTE_SIZE);
        std::ofstream file(sstFile->GetFileName(), std::ios::out | std::ios::binary);
        sstFile->WriteFile(file, dataToWrite, SearchType::BINARY_SEARCH, true);

        // Tests
        bool result = true;
        for (int i = 0; i < 3 * SST::KV_PAIRS_PER_PAGE; i++) {
            uint64_t value = sstFile->PerformBinarySearch(i, nullptr);
            result &= value == i + 10;
        }

        // search non-existing key
        uint64_t value = sstFile->PerformBinarySearch(3 * SST::KV_PAIRS_PER_PAGE + 10, nullptr);
        result &= value == Utils::INVALID_VALUE;

        // Clean up
        std::remove(filename.c_str());

        return result;
    }

    static bool TestPerformBinarySearchPartialSST() {
        // Setup
        std::string filename = Utils::GetFilenameWithExt("test");
        std::vector<DataEntry_t> dataToWrite;
        for (int i = 0; i < 10; i++) {
            dataToWrite.emplace_back(i, i + 10);
        }
        SST *sstFile = new SST(filename, dataToWrite.size() * SST::KV_PAIR_BYTE_SIZE);
        std::ofstream file(sstFile->GetFileName(), std::ios::out | std::ios::binary);
        sstFile->WriteFile(file, dataToWrite, SearchType::BINARY_SEARCH, true);

        // Tests
        bool result = true;

        // search existing keys
        for (int i = 0; i < 10; i++) {
            uint64_t value = sstFile->PerformBinarySearch(i, nullptr);
            result &= value == i + 10;
        }

        // search non-existing key
        uint64_t value = sstFile->PerformBinarySearch(20, nullptr);
        result &= value == Utils::INVALID_VALUE;

        // Clean up
        std::remove(filename.c_str());

        return result;
    }

    static bool TestPerformBinarySearchKeyNotExist() {
        // Setup
        std::string filename = Utils::GetFilenameWithExt("test");
        std::vector<DataEntry_t> dataToWrite;

        // Put even valued keys
        for (int i = 0; i < 2 * SST::KV_PAIRS_PER_PAGE; i += 2) {
            dataToWrite.emplace_back(i, i + 10);
        }
        SST *sstFile = new SST(filename, dataToWrite.size() * SST::KV_PAIR_BYTE_SIZE);
        std::ofstream file(sstFile->GetFileName(), std::ios::out | std::ios::binary);
        sstFile->WriteFile(file, dataToWrite, SearchType::BINARY_SEARCH, true);

        // Tests
        bool result = true;

        // Search for odd valued keys
        for (int i = 1; i < 2 * SST::KV_PAIRS_PER_PAGE; i += 2) {
            uint64_t value = sstFile->PerformBinarySearch(i, nullptr);
            result &= value == Utils::INVALID_VALUE;
        }

        // Clean up
        std::remove(filename.c_str());

        return result;
    }

    static bool TestPerformBinaryScanFullSST() {
        // Setup
        std::string filename = Utils::GetFilenameWithExt("test");
        std::vector<DataEntry_t> dataToWrite;
        uint64_t numEntries = 2 * SST::KV_PAIRS_PER_PAGE;
        for (int i = 10; i < numEntries + 10; i++) {
            dataToWrite.emplace_back(i, i + 10);
        }
        SST *sstFile = new SST(filename, dataToWrite.size() * SST::KV_PAIR_BYTE_SIZE);
        std::ofstream file(sstFile->GetFileName(), std::ios::out | std::ios::binary);
        sstFile->WriteFile(file, dataToWrite, SearchType::BINARY_SEARCH, true);

        // Tests
        bool result = true;

        // test scanning all data
        std::vector<DataEntry_t> dataScanned;
        sstFile->PerformBinaryScan(10, numEntries + 10, dataScanned);
        result &= dataScanned.size() == numEntries;
        for (uint64_t i = 10; i < numEntries + 10; i++) {
            result &= std::find(dataScanned.begin(), dataScanned.end(), std::make_pair(i, i + 10)) != dataScanned.end();
        }

        // test scanning part of data
        dataScanned.clear();
        uint64_t key1 = SST::KV_PAIRS_PER_PAGE + 100;
        uint64_t key2 = 2 * SST::KV_PAIRS_PER_PAGE - 1;
        sstFile->PerformBinaryScan(key1, key2, dataScanned);
        result &= dataScanned.size() == key2 - key1 + 1;
        uint64_t expectedScannedKeysSum = (key2 * (key2 + 1) / 2) - ((key1 - 1) * key1 / 2);
        uint64_t scannedKeysSum = 0;
        for (uint64_t i = key1; i <= key2; i++) {
            result &= std::find(dataScanned.begin(), dataScanned.end(), std::make_pair(i, i + 10)) != dataScanned.end();
            scannedKeysSum += i;
        }
        result &= expectedScannedKeysSum == scannedKeysSum;

        // test scanning with keys not in data
        dataScanned.clear();
        sstFile->PerformBinaryScan(0, numEntries + 20, dataScanned);
        result &= dataScanned.size() == numEntries;
        for (uint64_t i = 10; i < numEntries + 10; i++) {
            result &= std::find(dataScanned.begin(), dataScanned.end(), std::make_pair(i, i + 10)) != dataScanned.end();
        }

        // Clean up
        std::remove(filename.c_str());

        return result;
    }

    static bool TestPerformBinaryScanPartialSST() {
        // Setup
        std::string filename = Utils::GetFilenameWithExt("test");
        uint64_t numEntries = 2 * SST::KV_PAIRS_PER_PAGE - 10;
        std::vector<DataEntry_t> dataToWrite;
        for (int i = 0; i < numEntries; i++) {
            dataToWrite.emplace_back(i, i + 10);
        }
        SST *sstFile = new SST(filename, dataToWrite.size() * SST::KV_PAIR_BYTE_SIZE);
        std::ofstream file(sstFile->GetFileName(), std::ios::out | std::ios::binary);
        sstFile->WriteFile(file, dataToWrite, SearchType::BINARY_SEARCH, true);

        // Tests
        bool result = true;

        // test scanning all data
        std::vector<DataEntry_t> dataScanned;
        sstFile->PerformBinaryScan(0, numEntries, dataScanned);
        result &= dataScanned.size() == numEntries;
        for (uint64_t i = 0; i < numEntries; i++) {
            result &= std::find(dataScanned.begin(), dataScanned.end(), std::make_pair(i, i + 10)) != dataScanned.end();
        }

        // test scanning part of data
        dataScanned.clear();
        sstFile->PerformBinaryScan(10, 19, dataScanned);
        result &= dataScanned.size() == 10;
        for (uint64_t i = 10; i < 20; i++) {
            result &= std::find(dataScanned.begin(), dataScanned.end(), std::make_pair(i, i + 10)) != dataScanned.end();
        }

        // Clean up
        std::remove(filename.c_str());

        return result;
    }

   public:
    bool RunTests() override {
        bool allTestPassed = true;
        allTestPassed &= assertTrue(TestWriteFile, "TestSST::TestWriteFile");
        allTestPassed &= assertTrue(TestReadOnePageOfFile, "TestSST::TestReadOnePageOfFile");
        allTestPassed &= assertTrue(TestPerformBinarySearchFullSST, "TestSST::TestPerformBinarySearchFullSST");
        allTestPassed &= assertTrue(TestPerformBinarySearchPartialSST, "TestSST::TestPerformBinarySearchPartialSST");
        allTestPassed &= assertTrue(TestPerformBinarySearchKeyNotExist, "TestSST::TestPerformBinarySearchKeyNotExist");
        allTestPassed &= assertTrue(TestPerformBinaryScanFullSST, "TestSST::TestPerformBinaryScanFullSST");
        allTestPassed &= assertTrue(TestPerformBinaryScanPartialSST, "TestSST::TestPerformBinaryScanPartialSST");
        return allTestPassed;
    }
};
