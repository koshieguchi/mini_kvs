#ifndef SST_H
#define SST_H

#include <cstdint>
#include <fstream>
#include <list>
#include <map>
#include <queue>
#include <string>
#include <vector>

#include "BTreeLevel.h"
#include "BloomFilter.h"
#include "BufferPool.h"
#include "Utils.h"

class InputReader;

class ScanInputReader;

enum SearchType { BINARY_SEARCH = 0, B_TREE_SEARCH = 1 };

/**
 * Class representing a SST file in the database.
 */
class SST {
   private:
    std::string fileName;
    uint64_t fileDataByteSize;
    std::vector<BTreeLevel *> bTreeLevels;  // Used when the sst file is a static B-tree
    BloomFilter *bloomFilter;
    uint64_t maxOffsetToReadLeaves;
    InputReader *inputReader;
    ScanInputReader *scanInputReader;

    /**
     * Gets the the pageId of a page of a file to use as a key in the buffer pool.
     *
     * @param offsetToRead
     */
    std::string GetPageIdInBufferPool(uint64_t offsetToRead) {
        return this->fileName + "-" + std::to_string(offsetToRead);
    }

    static void WriteExtraToAlign(std::ofstream &file, uint64_t extraSpace);

    void AddNextInternalLevelFenceKeys(std::vector<uint64_t> &data, int nextLevel);

    /**
     * Write B-Tree internal nodes into the SST file.
     *
     * @param file the file stream of the SST file.
     * @param endOfFile flag to determine whether it's the end of the file or not.
     */
    void WriteBTreeInternalLevels(std::ofstream &file, bool endOfFile);

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

    std::vector<uint64_t> GetBTreeLevelOffsets(int leavesNumPages);

    /**
     * Try to obtain page from buffer pool with given page ID. If page isn't in the buffer
     * pool, try to obtain the page data from the file with given file description and offset.
     *
     * @param pageId the page ID of the page in buffer pool.
     * @param fd the file description of SST file containing the page.
     * @param offset the offset of the page in the SST file.
     * @param bufferPool the buffer pool.
     * @return a vector containing the page data.
     */
    static std::vector<uint64_t> GetPage(const std::string &pageId, int fd, uint64_t offset, BufferPool *bufferPool);

    static std::vector<uint64_t> GetBloomFilterPages(const std::string &pageId, int fd, uint64_t offset,
                                                     uint64_t numPages, BufferPool *bufferPool);

    /**
     * Read SST file to obtain given number of pages of bloom filters.
     *
     * @param fd the file descriptor of the SST file.
     * @param offset the offset in the file to the bloom filter.
     * @param numPagesToRead number of bloom filter pages to read.
     * @return bloom filter array read from the file.
     */
    static std::vector<uint64_t> ReadBloomFilter(int fd, uint64_t offset, uint64_t numPagesToRead);

    /**
     * Searches for key in the B-Tree file given the file offset and descriptor.
     *
     * @param fd the file descriptor of the B-Tree SST file.
     * @param key the key to search for.
     * @param bufferPool the DB buffer pool.
     * @param levelsPageOffsets the file offsets for B-Tree levels.
     * @return the value if key is found, INVALID_VALUE is not.
     */
    uint64_t FindKeyInBTree(int fd, uint64_t key, BufferPool *bufferPool, std::vector<uint64_t> &levelsPageOffsets);

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
     * @param fileName the name of the SST file.
     * @param fileDataByteSize the size of the file in bytes.
     * @param bloomFilter the bloom filter associated with the SST file.
     */
    explicit SST(std::string &fileName, uint64_t fileDataByteSize = 0, BloomFilter *bloomFilter = nullptr);

    ~SST() {
        delete this->bloomFilter;
        for (auto bTreeLevel : this->bTreeLevels) {
            delete bTreeLevel;
        }
        this->bTreeLevels.clear();
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
     * @param fileDataByteSize new file data size in bytes.
     */
    void SetFileDataSize(uint64_t fileDataByteSize);

    /**
     * Get the input buffer reader of the SST file.
     */
    InputReader *GetInputReader();

    /**
     * Set the input buffer reader of the SST file.
     *
     * @param inputReader new input buffer reader for SST file.
     */
    void SetInputReader(InputReader *inputReader);

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
     * @param inputReader new scan input buffer reader for SST file.
     */
    void SetScanInputReader(ScanInputReader *scanInputReader);

    /**
     * Set up the SST file for B-Tree data structure.
     */
    void SetupBTreeFile();

    /**
     * Write a B-Tree level into the SST file.
     *
     * @param file the file stream of the SST file.
     * @param data the level data to write.
     * @param endOfFile flag to determine whether it's the end of the file or not.
     */
    void WriteBTreeLevels(std::ofstream &file, std::vector<DataEntry_t> &data, bool endOfFile);

    /**
     * Write the end of the B-Tree SST file including B-Tree metadata and bloom filter.
     *
     * Clears bloom filter array afterwards to free up unused memory.
     *
     * @param file the file stream of the SST file.
     */
    void WriteEndOfBTreeFile(std::ofstream &file);

    /**
     * Write given key-value data into a SST file with given fileName.
     *
     * @param file the file stream of the SST file.
     * @param data the data to write into the file.
     * @param searchType the search type of the file (binary search or B-Tree search)
     * @param endOfFile flag to determine whether it's the end of the file or not.
     */
    void WriteFile(std::ofstream &file, std::vector<DataEntry_t> &data, SearchType searchType, bool endOfFile);

    /**
     * Read pages of data off of file with given file descriptor at given offset and
     * number of pages to read.
     *
     * @param fd the file description of SST file containing the page.
     * @param offset the offset of the page in the SST file.
     * @param numPagesToRead number of pages to read from the file.
     * @return a vector containing the page data read from file.
     */
    static std::vector<uint64_t> ReadPagesOfFile(int fd, uint64_t offset, uint64_t numPagesToRead = 1);

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
     * @param bufferPool the DB buffer pool.
     * @return the value if key is found, INVALID_VALUE otherwise.
     */
    uint64_t PerformBinarySearch(uint64_t key, BufferPool *bufferPool);

    /**
     * Queries for value with given key using B-Tree search.
     *
     * @param key the key to search for.
     * @param bufferPool the DB buffer pool.
     * @param isLSMTree flag to determine whether DB is using LSM-Tree or not.
     * @return the value if key is found, INVALID_VALUE otherwise.
     */
    uint64_t PerformBTreeSearch(uint64_t key, BufferPool *bufferPool, bool isLSMTree);

    /**
     * Scans for data whose key is within the range of [key1 and key2] using binary search.
     *
     * @param key1 the lower bound of scan result.
     * @param key2 the upper bound of scan result.
     * @param scanResult the vector to put scan results in.
     */
    void PerformBinaryScan(uint64_t key1, uint64_t key2, std::vector<DataEntry_t> &scanResult);

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
     * @param scanResult the vector to put scan results in.
     */
    void PerformBTreeScan(uint64_t key1, uint64_t key2, std::vector<DataEntry_t> &scanResult);
};

#endif  // SST_H
