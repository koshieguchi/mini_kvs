#include "scan_input_reader.h"

#include <iostream>

ScanInputReader::ScanInputReader(uint64_t capacity) {
    this->buffer_capacity = capacity;
    this->input_buffer = {};
    this->offset_to_read = 0;
    this->end_offset_to_scan = Utils::INVALID_VALUE;
    this->keys = {};
    this->start_index = 0;
    this->is_completely_scanned = false;
}

void ScanInputReader::ReadDataPagesIntoBuffer(int fd) {
    this->input_buffer.clear();
    this->keys.clear();
    if (this->offset_to_read > this->end_offset_to_scan) {
        this->is_completely_scanned = true;
        return;
    }

    uint64_t num_data_pages_to_read =
        std::min(this->buffer_capacity, this->end_offset_to_scan - this->offset_to_read + 1);
    this->input_buffer = SST::ReadPagesOfFile(fd, this->offset_to_read, num_data_pages_to_read);
    this->offset_to_read += num_data_pages_to_read;
    this->SetKeys();
    this->start_index = 0;
}

bool ScanInputReader::IsLeavesRangeToScanSet() const { return this->end_offset_to_scan != Utils::INVALID_VALUE; }

void ScanInputReader::SetLeavesRangeToScan(uint64_t new_start_offset_to_scan, uint64_t new_end_offset_to_scan, int fd) {
    this->offset_to_read = new_start_offset_to_scan;
    this->end_offset_to_scan = new_end_offset_to_scan;
    ScanInputReader::ReadDataPagesIntoBuffer(fd);
}

void ScanInputReader::SetKeys() {
    this->keys.clear();
    for (int i = 0; i < this->input_buffer.size(); i += 2) {
        this->keys.push_back(this->input_buffer[i]);
    }
}

int ScanInputReader::GetInputBufferSize() { return this->input_buffer.size(); }

DataEntry_t ScanInputReader::FindKey(uint64_t key, int fd) {
    // Set the default entry to INVALID_VALUE
    DataEntry_t entry = std::make_pair(key, Utils::INVALID_VALUE);
    int index = Utils::BinarySearch(this->keys, key, this->start_index);
    while (index >= this->keys.size()) {
        ScanInputReader::ReadDataPagesIntoBuffer(fd);
        if (this->keys.empty()) {
            return entry;
        }
        index = Utils::BinarySearch(this->keys, key, this->start_index);
    }

    this->start_index = index;
    if (this->keys[index] == key) {
        entry.second = this->input_buffer[2 * index + 1];
    }
    return entry;
}

bool ScanInputReader::IsScannedCompletely() const { return this->is_completely_scanned; }
