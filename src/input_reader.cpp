#include "input_reader.h"

#include <iostream>

InputReader::InputReader(uint64_t max_offset_to_read, int capacity) {
    this->input_buffer = {};
    this->offset_to_read = 0;
    this->buffer_capacity = capacity;
    this->max_offset_to_read = max_offset_to_read;
}

void InputReader::ObtainOffsetToRead(int fd) {
    this->level_offsets = SST::ReadBTreeLevelOffsets(fd);

    // Get the offset of where the B-tree's leaves starts
    this->offset_to_read = this->level_offsets[this->level_offsets.size() - 1];
}

void InputReader::ReadDataPagesInBuffer(int fd) {
    this->input_buffer.clear();
    if (this->offset_to_read > this->max_offset_to_read) {
        return;
    }

    uint64_t num_data_pages_to_read =
        std::min(this->buffer_capacity, this->max_offset_to_read - this->offset_to_read + 1);
    this->input_buffer = SST::ReadPagesOfFile(fd, this->offset_to_read, num_data_pages_to_read);
    this->offset_to_read += num_data_pages_to_read;
}

DataEntry_t InputReader::GetEntry(int index) {
    DataEntry_t entry = std::make_pair(this->input_buffer[index], this->input_buffer[index + 1]);
    if (this->input_buffer[index] == Utils::INVALID_VALUE) {
        // The number of entries in the file has not been page-aligned,
        // and we have reached the end of the file, so we should stop reading.
        this->input_buffer.clear();
    }
    return entry;
}

uint64_t InputReader::GetInputBufferSize() { return this->input_buffer.size(); }
