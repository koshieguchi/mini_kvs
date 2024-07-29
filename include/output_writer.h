#ifndef OUTPUT_WRITER_H
#define OUTPUT_WRITER_H

#include <fstream>
#include <vector>

#include "sst.h"
#include "utils.h"

/**
 * Class representing an Output Buffer writer for LSM-Tree.
 */
class OutputWriter {
   private:
    SST *sst_file;
    // Number of pages that output buffer hold in memory before writing it to storage
    int buffer_capacity;
    std::vector<DataEntry_t> output_buffer;
    std::ofstream file;
    int num_pages_written_to_file;

   public:
    /**
     * Constructor for a OutputWrite object.
     *
     * @param sst_file the SST file the buffer is associated with.
     * @param capacity the capacity of the buffer (in number of pages).
     */
    OutputWriter(SST *sst_file, int capacity);

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

#endif  // OUTPUT_WRITER_H
