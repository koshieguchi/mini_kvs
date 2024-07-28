#ifndef OUTPUTWRITER_H
#define OUTPUTWRITER_H

#include <fstream>
#include <vector>

#include "SST.h"
#include "Utils.h"

/**
 * Class representing an Output Buffer writer for LSM-Tree.
 */
class OutputWriter {
   private:
    SST *sstFile;
    // Number of pages that output buffer hold in memory before writing it to storage
    int bufferCapacity;
    std::vector<DataEntry_t> outputBuffer;
    std::ofstream file;
    int numPagesWrittenToFile;

   public:
    /**
     * Constructor for a OutputWrite object.
     *
     * @param sstFile the SST file the buffer is associated with.
     * @param capacity the capacity of the buffer (in number of pages).
     */
    OutputWriter(SST *sstFile, int capacity);

    /**
     * Add a new key-value entry into the buffer. Write to file if the buffer is full.
     *
     * @param entry the key-value entry.
     */
    void AddToOutputBuffer(DataEntry_t entry);

    /**
     * Write the end of the file current buffer is associated with.
     *
     * @return the number of pages written to the file.
     */
    int WriteEndOfFile();
};

#endif  // OUTPUTWRITER_H
