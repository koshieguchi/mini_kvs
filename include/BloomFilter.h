#ifndef BLOOMFILTER_H
#define BLOOMFILTER_H

#include <vector>

#include "Utils.h"
#include "xxhash.h"

/**
 * Class representing a Bloom Filter data structure.
 */
class BloomFilter {
   private:
    uint64_t arrayBitSize;
    uint64_t arraySize;
    int numHashFunctions;
    std::vector<uint64_t> array;

   public:
    /**
     * Constructor for a BloomFilter object.
     *
     * @param bitsPerEntry the number of bits in filter array used by each entry.
     * @param numKeys the max number of keys in the bloom filter.
     */
    explicit BloomFilter(int bitsPerEntry, int numKeys);

    static int GetIndexInFilterArray(uint64_t index);

    static uint64_t GetIndexInBitArray(uint64_t key, uint64_t seed, uint64_t arrayBitSize);

    /**
     * Insert a new key into the bloom filter.
     *
     * @param key the key to be inserted.
     */
    void InsertKey(uint64_t key);

    /**
     * Insert all the keys given into the bloom filter.
     *
     * @param data a vector containing all the keys to be inserted.
     */
    void InsertKeys(std::vector<DataEntry_t> &data);

    /**
     * Check if key exists in the bloom filter or not. Does not produce false negatives.
     *
     * @param key the key to search for.
     * @param filterArray the bloom filter array to search through.
     * @return false if the key doesn't exist, true if it probably exist.
     */
    [[nodiscard]] bool KeyProbablyExists(uint64_t key, std::vector<uint64_t> filterArray) const;

    static uint64_t GetShiftedLocationInBitArray(uint64_t index);

    /**
     * Get the entire bloom filter array.
     */
    std::vector<uint64_t> GetFilterArray();

    /**
     * Get the size of the bloom filter array as number of bits.
     */
    [[nodiscard]] uint64_t GetFilterArraySize() const;

    /**
     * Clear out the bloom filter array.
     */
    void ClearFilterArray();

    /**
     * Get the number of hash functions used by the bloom filter.
     */
    [[nodiscard]] int GetNumHashFunctions() const;
};

#endif  // BLOOMFILTER_H
