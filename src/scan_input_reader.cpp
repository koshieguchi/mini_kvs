#include "scan_input_reader.h"

#include <iostream>

ScanInputReader::ScanInputReader(uint64_t capacity) {
    this->bufferCapacity = capacity;
    this->inputBuffer = {};
    this->offsetToRead = 0;
    this->endOffsetToScan = Utils::INVALID_VALUE;
    this->keys = {};
    this->startIndex = 0;
    this->isScannedCompletely = false;
}

void ScanInputReader::ReadDataPagesIntoBuffer(int fd) {
    this->inputBuffer.clear();
    this->keys.clear();
    if (this->offsetToRead > this->endOffsetToScan) {
        this->isScannedCompletely = true;
        return;
    }

    uint64_t numDataPagesToRead = std::min(this->bufferCapacity, this->endOffsetToScan - this->offsetToRead + 1);
    this->inputBuffer = SST::ReadPagesOfFile(fd, this->offsetToRead, numDataPagesToRead);
    this->offsetToRead += numDataPagesToRead;
    this->SetKeys();
    this->startIndex = 0;
}

bool ScanInputReader::IsLeavesRangeToScanSet() const { return this->endOffsetToScan != Utils::INVALID_VALUE; }

void ScanInputReader::SetLeavesRangeToScan(uint64_t newStartOffsetToScan, uint64_t newEndOffsetToScan, int fd) {
    this->offsetToRead = newStartOffsetToScan;
    this->endOffsetToScan = newEndOffsetToScan;
    ScanInputReader::ReadDataPagesIntoBuffer(fd);
}

void ScanInputReader::SetKeys() {
    this->keys.clear();
    for (int i = 0; i < this->inputBuffer.size(); i += 2) {
        this->keys.push_back(this->inputBuffer[i]);
    }
}

int ScanInputReader::GetInputBufferSize() { return this->inputBuffer.size(); }

DataEntry_t ScanInputReader::FindKey(uint64_t key, int fd) {
    // Set the default entry to INVALID_VALUE
    DataEntry_t entry = std::make_pair(key, Utils::INVALID_VALUE);
    int index = Utils::BinarySearch(this->keys, key, this->startIndex);
    while (index >= this->keys.size()) {
        ScanInputReader::ReadDataPagesIntoBuffer(fd);
        if (this->keys.empty()) {
            return entry;
        }
        index = Utils::BinarySearch(this->keys, key, this->startIndex);
    }

    this->startIndex = index;
    if (this->keys[index] == key) {
        entry.second = this->inputBuffer[2 * index + 1];
    }
    return entry;
}

bool ScanInputReader::IsScannedCompletely() const { return this->isScannedCompletely; }
