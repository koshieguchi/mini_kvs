#ifndef LEVEL_H
#define LEVEL_H

#include <string>

#include "input_reader.h"
#include "output_writer.h"
#include "scan_input_reader.h"
#include "sst.h"
#include "utils.h"

/**
 * Class representing a level in the LSM-Tree data structure.
 */
class Level {
   private:
    int level;
    int bloom_filter_bits_per_entry;
    std::vector<SST *> sst_files;
    int input_buffer_capacity;
    int output_buffer_capacity;

    void AddSSTFile(SST *sst_file);

    void DeleteSSTFiles();

   public:
    /**
     * Constructor for a Level object.
     *
     * @param level the number of level it exists in the LSM-Tree.
     * @param bloom_filter_bits_per_entry the number of bits per entry for bloom filter.
     * @param input_buffer_capacity the capacity of input buffer in number of pages.
     * @param output_buffer_capacity the capacity of output buffer in number of pages.
     */
    Level(int level, int bloom_filter_bits_per_entry, int input_buffer_capacity, int output_buffer_capacity);

    // destructor
    ~Level();

    [[nodiscard]] int GetLevelNumber() const;

    /**
     * Write KV-pair data into current LSM-Tree level.
     *
     * @param data the KV-pair data.
     * @param search_type the search type used by KVS (binary search or B-Tree search)
     * @param kvs_path the path to the KVS file storage.
     */
    void WriteDataToLevel(std::vector<DataEntry_t> data, SearchType search_type, std::string &kvs_path);

    /**
     * Merge sort with the SST file at current level
     *
     * @param next_level the level in which sort-merged data will be written into
     * @param kvs_path the path to the KVS file storage.
     */
    void SortMergeAndWriteToNextLevel(Level *next_level, std::string &kvs_path);

    /**
     * Get all the SST file objects within current LSM-Tree level.
     */
    std::vector<SST *> GetSSTFiles();
};

#endif  // LEVEL_H
