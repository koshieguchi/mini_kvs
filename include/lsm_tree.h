#ifndef LSM_TREE_H
#define LSM_TREE_H

#include <string>
#include <vector>

#include "bloom_filter.h"
#include "buffer_pool.h"
#include "level.h"
#include "scan_input_reader.h"
#include "utils.h"

/**
 * Class representing a LSM-Tree data structure
 */
class LSMTree {
   private:
    std::vector<Level *> levels;  // vector of levels
    int bit_per_entry;
    int input_buffer_capacity;
    int output_buffer_capacity;

   public:
    /**
     * Constructor for a LSMTree object.
     *
     * @param bloom_filter_bits_per_entry the number of bits in filter array used by each entry.
     * @param input_buffer_capacity the capacity of input buffer in number of pages.
     * @param output_buffer_capacity the capacity of output buffer in number of pages.
     */
    explicit LSMTree(int bloom_filter_bits_per_entry, int input_buffer_capacity, int output_buffer_capacity);

    ~LSMTree();

    /**
     * Compact and push data into the next level if <cur_level> is full, otherwise do nothing.
     *
     * @param cur_level the current LSM-Tree level.
     * @param kvs_path the path to the KVS file storage.
     */
    void MaintainLevelCapacityAndCompact(Level *cur_level, std::string &kvs_path);

    /**
     * Write data in memtable into next level.
     *
     * @param data the data to write to file.
     * @param search_type the search type of the file (Binary search or B-Tree search).
     * @param kvs_path the path to the KVS file storage.
     */
    void WriteMemtableData(std::vector<DataEntry_t> &data, SearchType search_type, std::string &kvs_path);

    /**
     * Searches for value with given key in the LSM-Tree.
     *
     * @param key the key to search for.
     * @param buffer_pool the database buffer pool.
     * @return the value associated with the given key.
     */
    uint64_t Get(uint64_t key, BufferPool *buffer_pool = nullptr);

    /**
     * Scans for all data with keys within range of [key1, key2].
     *
     * @param key1 the lower bound of the scanned data.
     * @param key2 the upper bound of the scanned data.
     * @param scan_result the vector to put scanned results in.
     */
    void Scan(uint64_t key1, uint64_t key2, std::vector<DataEntry_t> &scan_result);

    // Methods used for testing purposes only
    std::vector<Level *> GetLevels();

    void AddLevel(Level *level);
};

#endif  // LSM_TREE_H
