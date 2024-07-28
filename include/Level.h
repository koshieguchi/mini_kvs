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
    int bloomFilterBitsPerEntry;
    std::vector<SST *> sstFiles;
    int inputBufferCapacity;
    int outputBufferCapacity;

    void AddSSTFile(SST *sstFile);

    void DeleteSSTFiles();

   public:
    /**
     * Constructor for a Level object.
     *
     * @param level the number of level it exists in the LSM-Tree.
     * @param bloomFilterBitsPerEntry the number of bits per entry for bloom filter.
     * @param inputBufferCapacity the capacity of input buffer in number of pages.
     * @param outputBufferCapacity the capacity of output buffer in number of pages.
     */
    Level(int level, int bloomFilterBitsPerEntry, int inputBufferCapacity, int outputBufferCapacity);

    // destructor
    ~Level();

    [[nodiscard]] int GetLevelNumber() const;

    /**
     * Write KV-pair data into current LSM-Tree level.
     *
     * @param data the KV-pair data.
     * @param searchType the search type used by DB (binary search or B-Tree search)
     * @param dbPath the path to the DB file storage.
     */
    void WriteDataToLevel(std::vector<DataEntry_t> data, SearchType searchType, std::string &dbPath);

    /**
     * Merge sort with the SST file at current level
     *
     * @param nextLevel the level in which sort-merged data will be written into
     * @param dbPath the path to the DB file storage.
     */
    void SortMergeAndWriteToNextLevel(Level *nextLevel, std::string &dbPath);

    /**
     * Get all the SST file objects within current LSM-Tree level.
     */
    std::vector<SST *> GetSSTFiles();
};

#endif  // LEVEL_H
