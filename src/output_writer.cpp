#include "output_writer.h"

#include <iostream>

OutputWriter::OutputWriter(SST *sst_file, int capacity) {
    this->sst_file = sst_file;
    this->file.open(sst_file->GetFileName(), std::ios::out | std::ios::binary);
    this->buffer_capacity = capacity * SST::KV_PAIRS_PER_PAGE;
    this->output_buffer = {};
    this->num_pages_written_to_file = 0;
}

void OutputWriter::AddToOutputBuffer(DataEntry_t entry) {
    this->output_buffer.push_back(entry);
    if (this->output_buffer.size() >= this->buffer_capacity) {
        this->sst_file->WriteBTreeLevels(this->file, this->output_buffer, false);
        this->num_pages_written_to_file += this->output_buffer.size() / SST::KV_PAIRS_PER_PAGE;
        this->output_buffer.clear();
    }
}

int OutputWriter::WriteEndOfFile() {
    if (!this->output_buffer.empty()) {
        this->sst_file->WriteBTreeLevels(this->file, this->output_buffer, false);
        this->num_pages_written_to_file += this->output_buffer.size() / SST::KV_PAIRS_PER_PAGE;
        this->output_buffer.clear();
    }
    this->sst_file->WriteEndOfBTreeFile(this->file);
    return this->num_pages_written_to_file;
}
