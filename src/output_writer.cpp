#include "output_writer.h"

#include <iostream>

OutputWriter::OutputWriter(SST *sstFile, int capacity) {
    this->sstFile = sstFile;
    this->file.open(sstFile->GetFileName(), std::ios::out | std::ios::binary);
    this->bufferCapacity = capacity * SST::KV_PAIRS_PER_PAGE;
    this->outputBuffer = {};
    this->numPagesWrittenToFile = 0;
}

void OutputWriter::AddToOutputBuffer(DataEntry_t entry) {
    this->outputBuffer.push_back(entry);
    if (this->outputBuffer.size() >= this->bufferCapacity) {
        this->sstFile->WriteBTreeLevels(this->file, this->outputBuffer, false);
        this->numPagesWrittenToFile += this->outputBuffer.size() / SST::KV_PAIRS_PER_PAGE;
        this->outputBuffer.clear();
    }
}

int OutputWriter::WriteEndOfFile() {
    if (!this->outputBuffer.empty()) {
        this->sstFile->WriteBTreeLevels(this->file, this->outputBuffer, false);
        this->numPagesWrittenToFile += this->outputBuffer.size() / SST::KV_PAIRS_PER_PAGE;
        this->outputBuffer.clear();
    }
    this->sstFile->WriteEndOfBTreeFile(this->file);
    return this->numPagesWrittenToFile;
}
