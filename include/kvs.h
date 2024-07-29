#ifndef KVS_H
#define KVS_H

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
class KVS {
   private:
    Memtable *memtable;
    std::string kvs_path;
    std::vector<SST *> all_ssts;
    BufferPool *buffer_pool;
    SearchType search_type;
    bool is_lsm_tree;
    LSMTree *lsm_tree;

   public:
    /**
     * Creates a new KVS object with given search type.
     *
     * @param memtable_size the maximum size of the memtable.
     * @param search_type the search type of SST files.
     * @param buffer_pool the buffer pool of the KVS.
     * @param lsm_tree the LSM-Tree data structure for the KVS.
     */
    explicit KVS(int memtable_size, SearchType search_type, BufferPool *buffer_pool = nullptr,
                 LSMTree *lsm_tree = nullptr);

    ~KVS();

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
     * @param new_value
     */
    void Update(uint64_t key, uint64_t new_value);

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
     * @param scan_result in scan_result with list of all keys between key1 and key2.
     */
    void Scan(uint64_t key1, uint64_t key2, std::vector<DataEntry_t> &scan_result);

    /**
     * Resets this kvs's buffer pool by creating a new extendible hashtable with new min size,
     * max size, and eviction policy for it.
     *
     * This method is used in experiments.
     *
     * @param buffer_pool_min_size
     * @param buffer_pool_max_size
     * @param eviction_policy_type
     */
    void ResetBufferPool(int buffer_pool_min_size, int buffer_pool_max_size, EvictionPolicyType eviction_policy_type);
};

#endif  // KVS_H
