
#include <iostream>
#include <set>
#include <vector>

#include "bloom_filter.h"
#include "test_base.h"

class TestBloomFilter : public TestBase {
    static const int bits_per_entry = 10;
    static const int num_keys = 131328;

    /**
     * Expect the index to which each key is hashed to be unique and within the range
     * of the bloom filter bit array size.
     */
    static bool TestGetIndexInBitArray() {
        // 1. Set up data
        auto *bloom_filter = new BloomFilter(bits_per_entry, num_keys);
        uint64_t bloom_filterarray_bit_size = bloom_filter->GetFilterArraySize() * Utils::EIGHT_BYTE_SIZE;

        // 2. Run and check the expected values
        bool result = true;
        for (uint64_t i = 0; i < 512; i++) {
            for (uint64_t t = i * 256; t < (i + 1) * 256; t++) {
                std::vector<uint64_t> hashedIndexes;
                int numHashFuncs = bloom_filter->GetNumHashFunctions();
                for (uint64_t seed = 1; seed <= numHashFuncs; seed += numHashFuncs) {
                    uint64_t index = BloomFilter::GetIndexInBitArray(t, seed, bloom_filterarray_bit_size);
                    hashedIndexes.push_back(index);
                    result &= index < bloom_filter->GetFilterArraySize() * 64;
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
        auto *bloom_filter = new BloomFilter(bits_per_entry, num_keys);
        uint64_t bloom_filterarray_bit_size = bloom_filter->GetFilterArraySize() * Utils::EIGHT_BYTE_SIZE;
        uint64_t key = 131327;

        // 2. Run and check the expected values
        bool result = true;
        int numHashFuncs = bloom_filter->GetNumHashFunctions();
        for (uint64_t seed = 1; seed <= numHashFuncs; seed += numHashFuncs) {
            uint64_t index = BloomFilter::GetIndexInBitArray(key, seed, bloom_filterarray_bit_size);
            bloom_filter->InsertKey(key);
            int i = BloomFilter::GetIndexInFilterArray(index);

            // Expect the index to have been set in the bloom_filter's bit array.
            result = result && (bloom_filter->GetFilterArray()[i] & BloomFilter::GetShiftedLocationInBitArray(index));
        }
        return result;
    }

    /**
     * Expect the bloom filter to return probably true for existing keys.
     */
    static bool TestKeyProbablyExists() {
        // 1. Set up data
        auto *bloom_filter = new BloomFilter(bits_per_entry, num_keys);
        for (uint64_t key = 0; key < 512; key += 2) {
            bloom_filter->InsertKey(key);
        }

        // 2. Run and check the expected values
        bool result = true;
        for (uint64_t key = 0; key < 512; key += 2) {
            result &= bloom_filter->KeyProbablyExists(key, bloom_filter->GetFilterArray());
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
