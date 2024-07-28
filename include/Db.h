#ifndef DB_H
#define DB_H

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "buffer_pool.h"
#include "lsm_tree.h"
#include "memtable.h"
#include "sst.h"

/**
 * Class representing the key-value database.
 */
class Db {
   private:
    Memtable *memtable;
    std::string dbPath;
    std::vector<SST *> allSSTs;
    BufferPool *bufferPool;
    SearchType searchType;
    bool isLSMTree;
    LSMTree *lsmTree;

   public:
    /**
     * Creates a new Db object with given search type.
     *
     * @param memtableSize the maximum size of the memtable.
     * @param searchType the search type of SST files.
     * @param bufferPool the buffer pool of the DB.
     * @param lsmTree the LSM-Tree data structure for the Db.
     */
    explicit Db(int memtableSize, SearchType searchType, BufferPool *bufferPool = nullptr, LSMTree *lsmTree = nullptr);

    ~Db();

    /**
     * Opens the database at given path and prepares it to run.
     *
     * @param path the path to the database file storage.
     */
    bool Open(const std::string &path);

    /**
     * Closes the database.
     */
    void Close();

    /**
     * Insert a key associated with a value into the database.
     *
     * @param key
     * @param value
     */
    void Put(uint64_t key, uint64_t value);

    /**
     * Retrieves a value associated with a given key in the database.
     *
     * @param key
     * @return the value mapped to given key.
     */
    uint64_t Get(uint64_t key);

    /**
     * Updates an existing key to have a new value in the database.
     *
     * Only available if database is initialized using LSMTree data structure.
     *
     * @param key
     * @param newValue
     */
    void Update(uint64_t key, uint64_t newValue);

    /**
     * Deletes an existing key in the database.
     *
     * Only available if database is initialized using LSMTree data structure.
     *
     * @param key
     */
    void Delete(uint64_t key);

    /**
     * Retrieves all KV-pairs in a key range in key order (key1 < key2)
     *
     * @param key1 the lower bound of the scan range.
     * @param key2 the upper bound of the scan range.
     * @param scanResult in scanResult with list of all keys between key1 and key2.
     */
    void Scan(uint64_t key1, uint64_t key2, std::vector<DataEntry_t> &scanResult);

    /**
     * Resets this db's buffer pool by creating a new extendible hashtable with new min size,
     * max size, and eviction policy for it.
     *
     * This method is used in experiments.
     *
     * @param bufferPoolMinSize
     * @param bufferPoolMaxSize
     * @param evictionPolicyType
     */
    void ResetBufferPool(int bufferPoolMinSize, int bufferPoolMaxSize, EvictionPolicyType evictionPolicyType);
};

#endif  // DB_H
