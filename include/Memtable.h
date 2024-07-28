#ifndef MEMTABLE_H
#define MEMTABLE_H

#include <vector>

#include "red_black_tree.h"
#include "utils.h"

/**
 * Class representing a Memtable in the database.
 */
class Memtable {
   private:
    /* data */
    RedBlackTree *redBlackTree;
    int maxSize;

   public:
    /**
     * Constructor for a Memtable object.
     *
     * @param maxSize the maximum key-value entries the memtable can hold.
     */
    explicit Memtable(int maxSize);

    ~Memtable();

    /**
     * Insert a key-value pair into the memtable.
     *
     * @param key the key to be inserted.
     * @param value the value associated with the key.
     * @return true if insertion was successful, false otherwise (e.g., memtable is full)
     */
    bool Put(uint64_t key, uint64_t value);

    /**
     * Queries the value associated with given key in the memtable.
     *
     * @param key the key to search.
     * @return value if key exists in memtable, Utils::INVALID_VALUE otherwise.
     */
    uint64_t Get(uint64_t key);

    /**
     * Gather all key-value pairs whose key is within range of <key1> and <key2>.
     *
     * @param key1 the lower bound of scanned result.
     * @param key2 the upper bound of scanned result.
     * @return a vector containing scan result.
     */
    std::vector<DataEntry_t> Scan(uint64_t key1, uint64_t key2);

    /**
     * Get all key-value pairs within the memtable in ascending sorted order.
     *
     * @return a vector containing all key-value pairs.
     */
    std::vector<DataEntry_t> GetAllData();

    /**
     * Get the maximum size limit for the memtable.
     */
    [[nodiscard]] int GetMaxSize() const;  // Max of number of (key, value) pairs in memtable

    /**
     * Get the number of key-value pairs currently in the memtable.
     */
    int GetCurrentSize();

    /**
     * Reset the memtable by clearing out all key-value pairs it currently contains.
     */
    void Reset();
};

#endif  // MEMTABLE_H
