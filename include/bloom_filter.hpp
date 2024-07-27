#ifndef BLOOM_FILTER_H
#define BLOOM_FILTER_H

#include "btree.hpp"
#include "utils.hpp"

class BloomFilter {
 private:
  int total_bits;  // total number of bits in bitmap

  // number of hash functions
  int hash_functions;

  int hash(uint32_t key, int seed);

  static const int MAX_BITMAP_SIZE =
      Utils::PAGE_SIZE - sizeof(int) - sizeof(BloomFilter::hash_functions);

 public:
  // m = number of bits per entry
  BloomFilter(int m, int num_entries);

  bool bitmap[MAX_BITMAP_SIZE];

  void insert(uint32_t key);
  bool get(uint32_t key);
};

#endif  // BLOOM_FILTER_H
