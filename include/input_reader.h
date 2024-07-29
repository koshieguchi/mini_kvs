#ifndef INPUT_READER_H
#define INPUT_READER_H

#include <queue>
#include <vector>

#include "sst.h"
#include "utils.h"

/**
 * Class representing an Input Reader Buffer writer for LSM-Tree.
 */
class InputReader {
   private:
    uint64_t offset_to_read;
    uint64_t max_offset_to_read;
    uint64_t buffer_capacity;
    std::vector<uint64_t> input_buffer;
    // Used in LSM tree sort-merge compaction
    std::vector<uint64_t> level_offsets;

   public:
    /**
     * Constructor for a InputReader object.
     *
     * @param max_offset_to_read the maximum offset in which the buffer can read until in the file.
     * @param capacity the capacity of the buffer (in number of pages).
     */
    InputReader(uint64_t max_offset_to_read, int capacity);

    ~InputReader() = default;

    /**
     * Set the file descriptor for the buffer and obtains the offsets of each LSM-Tree level
     * in the file.
     *
     * @param fd the file descriptor.
     */
    void ObtainOffsetToRead(int fd);

    /**
     * Read the data pages into the buffer using the given file descriptor.
     *
     * @param fd the file descriptor.
     */
    void ReadDataPagesInBuffer(int fd);

    /**
     * Get entry from with in the input reader buffer.
     *
     * @param index the index to the entry.
     * @return a key-value pair.
     */
    DataEntry_t GetEntry(int index);

    /**
     * Get the current size of the input buffer.
     */
    uint64_t GetInputBufferSize();
};

#endif  // INPUT_READER_H
