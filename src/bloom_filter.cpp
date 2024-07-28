#include "bloom_filter.h"

#include <cmath>
#include <string>

BloomFilter::BloomFilter(int bitsPerEntry, int numKeys) {
    this->arrayBitSize = (bitsPerEntry * numKeys) + ((bitsPerEntry * numKeys) % Utils::EIGHT_BYTE_SIZE);
    this->arraySize = this->arrayBitSize / Utils::EIGHT_BYTE_SIZE;
    this->array.resize(this->arraySize);
    std::fill(this->array.begin(), this->array.begin(), 0);
    this->numHashFunctions = std::ceil(log(2) * bitsPerEntry);
}

uint64_t BloomFilter::GetIndexInBitArray(uint64_t key, uint64_t seed, uint64_t arrayBitSize) {
    std::string keyString = std::to_string(key);
    void *const buffer = (void *const)keyString.c_str();
    uint64_t hash = XXH64(buffer, keyString.size(), seed);
    uint64_t index = hash % arrayBitSize;
    return index;
}

int BloomFilter::GetIndexInFilterArray(uint64_t index) { return std::ceil(index / Utils::EIGHT_BYTE_SIZE); }

uint64_t BloomFilter::GetShiftedLocationInBitArray(uint64_t index) {
    return 1 << (Utils::EIGHT_BYTE_SIZE - (index % Utils::EIGHT_BYTE_SIZE) - 1);
}

void BloomFilter::InsertKey(uint64_t key) {
    // Insert this key in the array by hashing it
    // numHashFunctions times to different indexes of array.
    for (int seed = 1; seed <= this->numHashFunctions; seed += this->numHashFunctions) {
        uint64_t index = GetIndexInBitArray(key, seed, this->arrayBitSize);
        int i = GetIndexInFilterArray(index);
        uint64_t target = this->array[i];

        // Set i-th bit (from MSB) of the target to be 1.
        target = target | GetShiftedLocationInBitArray(index);
        this->array[i] = target;
    }
}

void BloomFilter::InsertKeys(std::vector<DataEntry_t> &data) {
    for (auto pair : data) {
        uint64_t key = pair.first;
        BloomFilter::InsertKey(key);
    }
}

bool BloomFilter::KeyProbablyExists(uint64_t key, std::vector<uint64_t> filterArray) const {
    for (int seed = 1; seed <= this->numHashFunctions; seed += this->numHashFunctions) {
        uint64_t index = GetIndexInBitArray(key, seed, this->arrayBitSize);
        int i = GetIndexInFilterArray(index);
        uint64_t target = filterArray[i];

        // check if the i-th bit (from MSB) of the target is set or not.
        target = target & GetShiftedLocationInBitArray(index);
        if (!target) {
            return false;
        }
    }
    return true;
}

std::vector<uint64_t> BloomFilter::GetFilterArray() { return this->array; }

uint64_t BloomFilter::GetFilterArraySize() const { return this->arraySize; }

void BloomFilter::ClearFilterArray() { this->array.clear(); }

int BloomFilter::GetNumHashFunctions() const { return this->numHashFunctions; }
