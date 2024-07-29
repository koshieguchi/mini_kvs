#include "bloom_filter.h"

#include <cmath>
#include <string>

BloomFilter::BloomFilter(int bits_per_entry, int num_keys) {
    this->array_bit_size = (bits_per_entry * num_keys) + ((bits_per_entry * num_keys) % Utils::EIGHT_BYTE_SIZE);
    this->array_size = this->array_bit_size / Utils::EIGHT_BYTE_SIZE;
    this->array.resize(this->array_size);
    std::fill(this->array.begin(), this->array.begin(), 0);
    this->num_hash_functions = std::ceil(log(2) * bits_per_entry);
}

uint64_t BloomFilter::GetIndexInBitArray(uint64_t key, uint64_t seed, uint64_t array_bit_size) {
    std::string key_string = std::to_string(key);
    void *const buffer = (void *const)key_string.c_str();
    uint64_t hash = XXH64(buffer, key_string.size(), seed);
    uint64_t index = hash % array_bit_size;
    return index;
}

int BloomFilter::GetIndexInFilterArray(uint64_t index) { return std::ceil(index / Utils::EIGHT_BYTE_SIZE); }

uint64_t BloomFilter::GetShiftedLocationInBitArray(uint64_t index) {
    return 1 << (Utils::EIGHT_BYTE_SIZE - (index % Utils::EIGHT_BYTE_SIZE) - 1);
}

void BloomFilter::InsertKey(uint64_t key) {
    // Insert this key in the array by hashing it
    // num_hash_functions times to different indexes of array.
    for (int seed = 1; seed <= this->num_hash_functions; seed += this->num_hash_functions) {
        uint64_t index = GetIndexInBitArray(key, seed, this->array_bit_size);
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

bool BloomFilter::KeyProbablyExists(uint64_t key, std::vector<uint64_t> filter_array) const {
    for (int seed = 1; seed <= this->num_hash_functions; seed += this->num_hash_functions) {
        uint64_t index = GetIndexInBitArray(key, seed, this->array_bit_size);
        int i = GetIndexInFilterArray(index);
        uint64_t target = filter_array[i];

        // check if the i-th bit (from MSB) of the target is set or not.
        target = target & GetShiftedLocationInBitArray(index);
        if (!target) {
            return false;
        }
    }
    return true;
}

std::vector<uint64_t> BloomFilter::GetFilterArray() { return this->array; }

uint64_t BloomFilter::GetFilterArraySize() const { return this->array_size; }

void BloomFilter::ClearFilterArray() { this->array.clear(); }

int BloomFilter::GetNumHashFunctions() const { return this->num_hash_functions; }
