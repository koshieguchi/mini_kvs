#include "bloom_filter.hpp"

#include <cmath>
#include <string>

#include "xxhash.h"

BloomFilter::BloomFilter(int m, int num_entries) {
  this->total_bits = m * num_entries >= MAX_BITMAP_SIZE ? MAX_BITMAP_SIZE
                                                        : m * num_entries;
  for (int i = 0; i < total_bits; i++) {
    this->bitmap[i] = 0;
  }
  // compute optimal number of hash functions
  this->hash_functions = std::round(std::log(2) * m);
}

void BloomFilter::insert(uint32_t key) {
  // we simulate different hash functions by choosing a different seed for each
  // one
  for (int i = 0; i < this->hash_functions; i++) {
    this->bitmap[this->hash(key, i)] = 1;
  }
}

bool BloomFilter::get(uint32_t key) {
  for (int i = 0; i < this->hash_functions; i++) {
    if (this->bitmap[this->hash(key, i)] == 0) {
      return false;
    }
  }
  return true;
}

int BloomFilter::hash(uint32_t key, int seed) {
  return XXH64(&key, sizeof(uint32_t), seed) % this->total_bits;
}
