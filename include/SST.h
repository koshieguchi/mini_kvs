#ifndef SST_H
#define SST_H

#include <cstdint>
#include <fstream>
#include <list>
#include <map>
#include <queue>
#include <string>
#include <vector>

#include "bloom_filter.h"
#include "btree_level.h"
#include "buffer_pool.h"
#include "utils.h"

class InputReader;

class ScanInputReader;

enum SearchType { BINARY_SEARCH = 0, B_TREE_SEARCH = 1 };

/**
 * Class representing a SST file in the database.
 */
class SST {
   private:
    std::string file_name;
    uint64_t file_data_byte_size;
    std::vector<BTreeLevel *> b_tree_levels;  // Used when the sst file is a static B-tree
    BloomFilter *bloom_filter;
    uint64_t max_offset_to_readLeaves;
    InputReader *input_reader;
    ScanInputReader *scan_input_reader;

    /**
     * Gets the the page_id of a page of a file to use as a key in the buffer pool.
     *
     * @param offset_to_read
     */
    std::string GetPageIdInBufferPool(uint64_t offset_to_read) {
        return this->file_name + "-" + std::to_string(offset_to_read);
    }

    static void WriteExtraToAlign(std::ofstream &file, uint64_t extraSpace);

    void AddNextInternalLevelFenceKeys(std::vector<uint64_t> &data, int next_level);

    /**
     * Write B-Tree internal nodes into the SST file.
     *
     * @param file the file stream of the SST file.
     * @param end_of_file flag to determine whether it's the end of the file or not.
     */
    void WriteBTreeInternalLevels(std::ofstream &file, bool end_of_file);

    /**
     * Write B-Tree structure metadata to the SST file.
     *
     * @param file the file stream of the SST file.
     */
    void WriteBTreeMetaData(std::ofstream &file);

    /**
     * Write bloom filter data into the SST file.
     *
     * @param file the file stream of the SST file.
     */
    void WriteBloomFilter(std::ofstream &file);

    std::vector<uint64_t> GetBTreeLevelOffsets(int leaves_num_pages);

    /**
     * Try to obtain page from buffer pool with given page ID. If page isn't in the buffer
     * pool, try to obtain the page data from the file with given file description and offset.
     *
     * @param page_id the page ID of the page in buffer pool.
     * @param fd the file description of SST file containing the page.
     * @param offset the offset of the page in the SST file.
     * @param buffer_pool the buffer pool.
     * @return a vector containing the page data.
     */
    static std::vector<uint64_t> GetPage(const std::string &page_id, int fd, uint64_t offset, BufferPool *buffer_pool);

    static std::vector<uint64_t> GetBloomFilterPages(const std::string &page_id, int fd, uint64_t offset,
                                                     uint64_t num_pages, BufferPool *buffer_pool);

    /**
     * Read SST file to obtain given number of pages of bloom filters.
     *
     * @param fd the file descriptor of the SST file.
     * @param offset the offset in the file to the bloom filter.
     * @param num_pages_to_read number of bloom filter pages to read.
     * @return bloom filter array read from the file.
     */
    static std::vector<uint64_t> ReadBloomFilter(int fd, uint64_t offset, uint64_t num_pages_to_read);

    /**
     * Searches for key in the B-Tree file given the file offset and descriptor.
     *
     * @param fd the file descriptor of the B-Tree SST file.
     * @param key the key to search for.
     * @param buffer_pool the DB buffer pool.
     * @param levels_page_offsets the file offsets for B-Tree levels.
     * @return the value if key is found, INVALID_VALUE is not.
     */
    uint64_t FindKeyInBTree(int fd, uint64_t key, BufferPool *buffer_pool, std::vector<uint64_t> &levels_page_offsets);

   public:
    // Size of one page of memory
    static const size_t PAGE_SIZE = 4096;
    static const size_t KV_PAIR_BYTE_SIZE = 16;
    static const size_t KEY_BYTE_SIZE = 8;
    static const size_t KV_PAIRS_PER_PAGE = PAGE_SIZE / 16;
    static const size_t KEYS_PER_PAGE = PAGE_SIZE / 8;

    /**
     * Constructor for a SST object.
     *
     * @param file_name the name of the SST file.
     * @param file_data_byte_size the size of the file in bytes.
     * @param bloom_filter the bloom filter associated with the SST file.
     */
    explicit SST(std::string &file_name, uint64_t file_data_byte_size = 0, BloomFilter *bloom_filter = nullptr);

    ~SST() {
        delete this->bloom_filter;
        for (auto b_tree_level : this->b_tree_levels) {
            delete b_tree_level;
        }
        this->b_tree_levels.clear();
    }

    /**
     * Get the file name of the SST file.
     */
    std::string GetFileName();

    /**
     * Get the data size of the SST file in bytes.
     */
    [[nodiscard]] uint64_t GetFileDataSize() const;

    /**
     * Set the data size of the SST file in bytes.
     *
     * @param file_data_byte_size new file data size in bytes.
     */
    void SetFileDataSize(uint64_t file_data_byte_size);

    /**
     * Get the input buffer reader of the SST file.
     */
    InputReader *GetInputReader();

    /**
     * Set the input buffer reader of the SST file.
     *
     * @param input_reader new input buffer reader for SST file.
     */
    void SetInputReader(InputReader *input_reader);

    /**
     * Get maximum offset to read the leaves level of the B-Tree structure for
     * current SST file.
     */
    [[nodiscard]] uint64_t GetMaxOffsetToReadLeaves() const;

    /**
     * Get the scan input buffer reader of the SST file.
     */
    ScanInputReader *GetScanInputReader();

    /**
     * Set the scan input buffer reader of the SST file.
     *
     * @param input_reader new scan input buffer reader for SST file.
     */
    void SetScanInputReader(ScanInputReader *scan_input_reader);

    /**
     * Set up the SST file for B-Tree data structure.
     */
    void SetupBTreeFile();

    /**
     * Write a B-Tree level into the SST file.
     *
     * @param file the file stream of the SST file.
     * @param data the level data to write.
     * @param end_of_file flag to determine whether it's the end of the file or not.
     */
    void WriteBTreeLevels(std::ofstream &file, std::vector<DataEntry_t> &data, bool end_of_file);

    /**
     * Write the end of the B-Tree SST file including B-Tree metadata and bloom filter.
     *
     * Clears bloom filter array afterwards to free up unused memory.
     *
     * @param file the file stream of the SST file.
     */
    void WriteEndOfBTreeFile(std::ofstream &file);

    /**
     * Write given key-value data into a SST file with given file_name.
     *
     * @param file the file stream of the SST file.
     * @param data the data to write into the file.
     * @param search_type the search type of the file (binary search or B-Tree search)
     * @param end_of_file flag to determine whether it's the end of the file or not.
     */
    void WriteFile(std::ofstream &file, std::vector<DataEntry_t> &data, SearchType search_type, bool end_of_file);

    /**
     * Read pages of data off of file with given file descriptor at given offset and
     * number of pages to read.
     *
     * @param fd the file description of SST file containing the page.
     * @param offset the offset of the page in the SST file.
     * @param num_pages_to_read number of pages to read from the file.
     * @return a vector containing the page data read from file.
     */
    static std::vector<uint64_t> ReadPagesOfFile(int fd, uint64_t offset, uint64_t num_pages_to_read = 1);

    /**
     * Read SST file to obtain B-Tree level offsets metadata.
     *
     * @param fd the SST file descriptor.
     * @return a vector containing the B-Tree level offsets metadata.
     */
    static std::vector<uint64_t> ReadBTreeLevelOffsets(int fd);

    /**
     * Queries for value with given key using binary search.
     *
     * @param key the key to search for.
     * @param buffer_pool the DB buffer pool.
     * @return the value if key is found, INVALID_VALUE otherwise.
     */
    uint64_t PerformBinarySearch(uint64_t key, BufferPool *buffer_pool);

    /**
     * Queries for value with given key using B-Tree search.
     *
     * @param key the key to search for.
     * @param buffer_pool the DB buffer pool.
     * @param is_lsm_tree flag to determine whether DB is using LSM-Tree or not.
     * @return the value if key is found, INVALID_VALUE otherwise.
     */
    uint64_t PerformBTreeSearch(uint64_t key, BufferPool *buffer_pool, bool is_lsm_tree);

    /**
     * Scans for data whose key is within the range of [key1 and key2] using binary search.
     *
     * @param key1 the lower bound of scan result.
     * @param key2 the upper bound of scan result.
     * @param scan_result the vector to put scan results in.
     */
    void PerformBinaryScan(uint64_t key1, uint64_t key2, std::vector<DataEntry_t> &scan_result);

    /**
     * Read SST file to obtain starting offset in leaves level in B-Tree structure for
     * scan operation.
     *
     * @param fd the file descriptor of the SST file.
     * @param key1 the starting range of scan operation.
     * @return the offset in leaves level in B-Tree in which scan operation begins.
     */
    static uint64_t ReadBTreeScanLeavesRange(int fd, uint64_t key1);

    /**
     * Scans for data whose key is within the range of [key1 and key2] using B-Tree search.
     *
     * @param key1 the lower bound of scan result.
     * @param key2 the upper bound of scan result.
     * @param scan_result the vector to put scan results in.
     */
    void PerformBTreeScan(uint64_t key1, uint64_t key2, std::vector<DataEntry_t> &scan_result);
};

#endif  // SST_H
