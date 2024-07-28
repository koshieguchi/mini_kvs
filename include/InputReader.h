#ifndef INPUTREADER_H
#define INPUTREADER_H

#include <queue>
#include <vector>

#include "SST.h"
#include "Utils.h"

/**
 * Class representing an Input Reader Buffer writer for LSM-Tree.
 */
class InputReader {
   private:
    uint64_t offsetToRead;
    uint64_t maxOffsetToRead;
    uint64_t bufferCapacity;
    std::vector<uint64_t> inputBuffer;
    // Used in LSM tree sort-merge compaction
    std::vector<uint64_t> levelOffsets;

   public:
    /**
     * Constructor for a InputReader object.
     *
     * @param maxOffsetToRead the maximum offset in which the buffer can read until in the file.
     * @param capacity the capacity of the buffer (in number of pages).
     */
    InputReader(uint64_t maxOffsetToRead, int capacity);

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

#endif  // INPUTREADER_H
