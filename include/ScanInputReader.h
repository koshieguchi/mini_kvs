#ifndef SACNINPUTREADER_H
#define SACNINPUTREADER_H

#include <iostream>
#include <queue>
#include <vector>

#include "SST.h"
#include "Utils.h"

/**
 * Class representing a Input Reader Buffer for LSM-Tree Scan operation.
 */
class ScanInputReader {
   private:
    std::vector<uint64_t> inputBuffer;
    uint64_t offsetToRead;
    uint64_t bufferCapacity;
    std::vector<uint64_t> keys;
    int startIndex;
    uint64_t endOffsetToScan;
    bool isScannedCompletely;

    void ReadDataPagesIntoBuffer(int fd);

    void SetKeys();

   public:
    /**
     * Constructor for a ScanInputReader object.
     *
     * @param capacity the capacity of the buffer (in number of pages).
     */
    explicit ScanInputReader(uint64_t capacity);

    ~ScanInputReader() = default;

    [[nodiscard]] bool IsLeavesRangeToScanSet() const;

    void SetLeavesRangeToScan(uint64_t startOffsetToScan, uint64_t endOffsetToScan, int fd);

    int GetInputBufferSize();

    /**
     * Reads data pages from file into the buffer and find KV-pair within the buffer
     * using given key.
     *
     * @param key the key to search for.
     * @param fd the file descriptor of the file.
     * @return KV pair if found, if not, a pair of key and INVALID_VALUE.
     */
    DataEntry_t FindKey(uint64_t key, int fd);

    [[nodiscard]] bool IsScannedCompletely() const;
};

#endif  // SACNINPUTREADER_H
