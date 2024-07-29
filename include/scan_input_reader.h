#ifndef SCAN_INPUT_READER_H
#define SCAN_INPUT_READER_H

#include <iostream>
#include <queue>
#include <vector>

#include "sst.h"
#include "utils.h"

/**
 * Class representing a Input Reader Buffer for LSM-Tree Scan operation.
 */
class ScanInputReader {
   private:
    std::vector<uint64_t> input_buffer;
    uint64_t offset_to_read;
    uint64_t buffer_capacity;
    std::vector<uint64_t> keys;
    int start_index;
    uint64_t end_offset_to_scan;
    bool is_completely_scanned;

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

    void SetLeavesRangeToScan(uint64_t start_offset_to_scan, uint64_t end_offset_to_scan, int fd);

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

#endif  // SCAN_INPUT_READER_H
