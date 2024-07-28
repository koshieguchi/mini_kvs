#include "InputReader.h"
#include <iostream>

InputReader::InputReader(uint64_t maxOffsetToRead, int capacity) {
    this->inputBuffer = {};
    this->offsetToRead = 0;
    this->bufferCapacity = capacity;
    this->maxOffsetToRead = maxOffsetToRead;
}

void InputReader::ObtainOffsetToRead(int fd) {
    this->levelOffsets = SST::ReadBTreeLevelOffsets(fd);

    // Get the offset of where the B-tree's leaves starts
    this->offsetToRead = this->levelOffsets[this->levelOffsets.size() - 1];
}

void InputReader::ReadDataPagesInBuffer(int fd) {
    this->inputBuffer.clear();
    if (this->offsetToRead > this->maxOffsetToRead) {
        return;
    }

    uint64_t numDataPagesToRead = std::min(this->bufferCapacity, this->maxOffsetToRead - this->offsetToRead + 1);
    this->inputBuffer = SST::ReadPagesOfFile(fd, this->offsetToRead, numDataPagesToRead);
    this->offsetToRead += numDataPagesToRead;
}

DataEntry_t InputReader::GetEntry(int index) {
    DataEntry_t entry = std::make_pair(this->inputBuffer[index], this->inputBuffer[index + 1]);
    if (this->inputBuffer[index] == Utils::INVALID_VALUE) {
        // The number of entries in the file has not been page-aligned,
        // and we have reached the end of the file, so we should stop reading.
        this->inputBuffer.clear();
    }
    return entry;
}

uint64_t InputReader::GetInputBufferSize() {
    return this->inputBuffer.size();
}