
#include <vector>
#include <set>
#include <iostream>
#include "TestBase.h"
#include "BloomFilter.h"

class TestBloomFilter : public TestBase {
    static const int bitsPerEntry = 10;
    static const int numKeys = 131328;

    /**
     * Expect the index to which each key is hashed to be unique and within the range
     * of the bloom filter bit array size.
     */
    static bool TestGetIndexInBitArray() {
        // 1. Set up data
        auto *bloomFilter = new BloomFilter(bitsPerEntry, numKeys);
        uint64_t bloomFilterArrayBitSize = bloomFilter->GetFilterArraySize() * Utils::EIGHT_BYTE_SIZE;

        // 2. Run and check the expected values
        bool result = true;
        for (uint64_t i = 0; i < 512; i++) {
            for (uint64_t t = i * 256; t < (i + 1) * 256; t++) {
                std::vector<uint64_t> hashedIndexes;
                int numHashFuncs = bloomFilter->GetNumHashFunctions();
                for (uint64_t seed = 1; seed <= numHashFuncs; seed += numHashFuncs) {
                    uint64_t index = BloomFilter::GetIndexInBitArray(t, seed, bloomFilterArrayBitSize);
                    hashedIndexes.push_back(index);
                    result &= index < bloomFilter->GetFilterArraySize() * 64;
                }
                std::set<uint64_t> hashedIndexesSet(hashedIndexes.begin(), hashedIndexes.end());
                if (hashedIndexes.size() != hashedIndexesSet.size()) {
                    std::cout << "Some hash functions hashed the key to the same index of the bloom filter array."
                              << std::endl;
                }
                result &= hashedIndexes.size() == hashedIndexesSet.size();
            }
        }
        return result;
    }

    /**
     * Expect the key to be hashed to different indexes of the bit array.
     */
    static bool TestInsertKey() {
        // 1. Set up data
        auto *bloomFilter = new BloomFilter(bitsPerEntry, numKeys);
        uint64_t bloomFilterArrayBitSize = bloomFilter->GetFilterArraySize() * Utils::EIGHT_BYTE_SIZE;
        uint64_t key = 131327;

        // 2. Run and check the expected values
        bool result = true;
        int numHashFuncs = bloomFilter->GetNumHashFunctions();
        for (uint64_t seed = 1; seed <= numHashFuncs; seed += numHashFuncs) {
            uint64_t index = BloomFilter::GetIndexInBitArray(key, seed, bloomFilterArrayBitSize);
            bloomFilter->InsertKey(key);
            int i = BloomFilter::GetIndexInFilterArray(index);

            // Expect the index to have been set in the bloomFilter's bit array.
            result = result && (bloomFilter->GetFilterArray()[i] & BloomFilter::GetShiftedLocationInBitArray(index));
        }
        return result;
    }

    /**
     * Expect the bloom filter to return probably true for existing keys.
     */
    static bool TestKeyProbablyExists() {
        // 1. Set up data
        auto *bloomFilter = new BloomFilter(bitsPerEntry, numKeys);
        for (uint64_t key = 0; key < 512; key += 2) {
            bloomFilter->InsertKey(key);
        }

        // 2. Run and check the expected values
        bool result = true;
        for (uint64_t key = 0; key < 512; key += 2) {
            result &= bloomFilter->KeyProbablyExists(key, bloomFilter->GetFilterArray());
        }
        return result;
    }

public:
    bool RunTests() override {
        bool allTestPassed = true;
        allTestPassed &= assertTrue(TestGetIndexInBitArray, "TestBloomFilter::TestGetIndexInBitArray");
        allTestPassed &= assertTrue(TestInsertKey, "TestBloomFilter::TestInsertKey");
        allTestPassed &= assertTrue(TestKeyProbablyExists, "TestBloomFilter::TestKeyProbablyExists");
        return allTestPassed;
    }
};
