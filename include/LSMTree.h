#ifndef LSMTREE_H
#define LSMTREE_H

#include <vector>
#include <string>
#include "Level.h"
#include "Utils.h"
#include "BufferPool.h"
#include "BloomFilter.h"
#include "ScanInputReader.h"

/**
 * Class representing a LSM-Tree data structure
 */
class LSMTree {
private:
    std::vector<Level *> levels; // vector of levels
    int bitPerEntry;
    int inputBufferCapacity;
    int outputBufferCapacity;

public:
    /**
     * Constructor for a LSMTree object.
     *
     * @param bloomFilterBitsPerEntry the number of bits in filter array used by each entry.
     * @param inputBufferCapacity the capacity of input buffer in number of pages.
     * @param outputBufferCapacity the capacity of output buffer in number of pages.
     */
    explicit LSMTree(int bloomFilterBitsPerEntry, int inputBufferCapacity, int outputBufferCapacity);

    ~LSMTree();

    /**
     * Compact and push data into the next level if <currLevel> is full, otherwise do nothing.
     *
     * @param currLevel the current LSM-Tree level.
     * @param dbPath the path to the DB file storage.
     */
    void MaintainLevelCapacityAndCompact(Level *currLevel, std::string &dbPath);

    /**
     * Write data in memtable into next level.
     *
     * @param data the data to write to file.
     * @param searchType the search type of the file (Binary search or B-Tree search).
     * @param dbPath the path to the DB file storage.
     */
    void WriteMemtableData(std::vector<DataEntry_t> &data, SearchType searchType, std::string &dbPath);

    /**
     * Searches for value with given key in the LSM-Tree.
     *
     * @param key the key to search for.
     * @param bufferPool the database buffer pool.
     * @return the value associated with the given key.
     */
    uint64_t Get(uint64_t key, BufferPool *bufferPool = nullptr);

    /**
     * Scans for all data with keys within range of [key1, key2].
     *
     * @param key1 the lower bound of the scanned data.
     * @param key2 the upper bound of the scanned data.
     * @param scanResult the vector to put scanned results in.
     */
    void Scan(uint64_t key1, uint64_t key2, std::vector<DataEntry_t> &scanResult);

    // Methods used for testing purposes only
    std::vector<Level *> GetLevels();

    void AddLevel(Level *level);

};

#endif // LSMTREE_H