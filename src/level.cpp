#include "level.h"

#include <unistd.h>

#include <cmath>
#include <filesystem>
#include <iostream>

Level::Level(int level, int bloom_filter_bits_per_entry, int input_buffer_capacity, int output_buffer_capacity) {
    this->level = level;
    this->bloom_filter_bits_per_entry = bloom_filter_bits_per_entry;
    this->sst_files = {};
    this->input_buffer_capacity = input_buffer_capacity;
    this->output_buffer_capacity = output_buffer_capacity;
}

Level::~Level() {
    for (auto sst_file : this->sst_files) {
        delete sst_file;
    }
}

int Level::GetLevelNumber() const { return this->level; }

void Level::WriteDataToLevel(std::vector<DataEntry_t> data, SearchType search_type, std::string &kvs_path) {
    std::string file_name = Utils::GetFilenameWithExt(std::to_string(this->sst_files.size()));
    std::string file_path = kvs_path + "/" + Utils::LEVEL + std::to_string(this->level) + "-" + file_name;
    uint64_t data_byte_size = data.size() * SST::KV_PAIR_BYTE_SIZE;
    BloomFilter *bloom_filter = new BloomFilter(this->bloom_filter_bits_per_entry, data.size());
    bloom_filter->InsertKeys(data);
    SST *sst_file = new SST(file_path, data_byte_size, bloom_filter);
    sst_file->SetupBTreeFile();
    sst_file->SetInputReader(new InputReader(sst_file->GetMaxOffsetToReadLeaves(), this->input_buffer_capacity));
    sst_file->SetScanInputReader(new ScanInputReader(this->input_buffer_capacity));
    std::ofstream file(sst_file->GetFileName(), std::ios::out | std::ios::binary);
    sst_file->WriteFile(file, data, search_type, true);
    this->sst_files.push_back(sst_file);
}

void WriteRemainingData(int fd, int index, BloomFilter *bloom_filter, InputReader *reader,
                        OutputWriter *output_writer) {
    while (index < reader->GetInputBufferSize()) {
        DataEntry_t entry = reader->GetEntry(index);
        output_writer->AddToOutputBuffer(entry);
        bloom_filter->InsertKey(entry.first);
        index += 2;
        // In the edge case when sst1_reader has greater keys than the sst2_reader,
        // make sure you read all the contents of sst1_reader.
        if (index >= reader->GetInputBufferSize()) {
            reader->ReadDataPagesInBuffer(fd);
            index = 0;
        }
    }
}

void Level::SortMergeAndWriteToNextLevel(Level *next_level, std::string &kvs_path) {
    uint64_t sst_data_size = 0;
    for (auto sst_file : this->sst_files) {
        sst_data_size += sst_file->GetFileDataSize();
    }

    // Create and setup new SST file for sort-merged data
    std::string file_name = Utils::GetFilenameWithExt(std::to_string(next_level->sst_files.size()));
    std::string file_path = kvs_path + "/" + Utils::LEVEL + std::to_string(next_level->level) + "-" + file_name;
    int max_num_keys = std::ceil(sst_data_size / SST::KV_PAIR_BYTE_SIZE);
    auto *bloom_filter = new BloomFilter(this->bloom_filter_bits_per_entry, max_num_keys);
    SST *sort_merged_file = new SST(file_path, sst_data_size, bloom_filter);

    sort_merged_file->SetupBTreeFile();
    sort_merged_file->SetInputReader(
        new InputReader(sort_merged_file->GetMaxOffsetToReadLeaves(), this->input_buffer_capacity));
    sort_merged_file->SetScanInputReader(new ScanInputReader(this->input_buffer_capacity));
    next_level->AddSSTFile(sort_merged_file);

    // Sort-merge data
    int fd1 = Utils::OpenFile(this->sst_files[0]->GetFileName());
    if (fd1 == -1) {
        return;
    }

    int fd2 = Utils::OpenFile(this->sst_files[1]->GetFileName());
    if (fd2 == -1) {
        return;
    }

    // InputReader will read 1 page of data from files at a time
    InputReader *sst1_reader = this->sst_files[0]->GetInputReader();
    sst1_reader->ObtainOffsetToRead(fd1);
    InputReader *sst2_reader = this->sst_files[1]->GetInputReader();
    sst2_reader->ObtainOffsetToRead(fd2);

    // Consider the output buffer size to be this->buffer_capacity page
    auto *output_writer = new OutputWriter(sort_merged_file, this->output_buffer_capacity);

    sst1_reader->ReadDataPagesInBuffer(fd1);
    sst2_reader->ReadDataPagesInBuffer(fd2);
    int index1 = 0;
    int index2 = 0;
    while (sst1_reader->GetInputBufferSize() && sst2_reader->GetInputBufferSize()) {
        DataEntry_t entry1 = sst1_reader->GetEntry(index1);
        DataEntry_t entry2 = sst2_reader->GetEntry(index2);
        if (entry1.first < entry2.first) {
            output_writer->AddToOutputBuffer(entry1);
            bloom_filter->InsertKey(entry1.first);
            index1 += 2;
        } else if (entry2.first < entry1.first) {
            output_writer->AddToOutputBuffer(entry2);
            bloom_filter->InsertKey(entry2.first);
            index2 += 2;
        } else {
            // It is an update/delete
            output_writer->AddToOutputBuffer(entry2);
            bloom_filter->InsertKey(entry2.first);
            index1 += 2;
            index2 += 2;
        }

        if (index1 >= sst1_reader->GetInputBufferSize()) {
            sst1_reader->ReadDataPagesInBuffer(fd1);
            index1 = 0;
        }
        if (index2 >= sst2_reader->GetInputBufferSize()) {
            sst2_reader->ReadDataPagesInBuffer(fd2);
            index2 = 0;
        }
    }

    // Write all the elements of the dataBuffer that still has remaining data to the output buffer
    if (sst1_reader->GetInputBufferSize()) {
        WriteRemainingData(fd1, index1, bloom_filter, sst1_reader, output_writer);
    } else if (sst2_reader->GetInputBufferSize()) {
        WriteRemainingData(fd2, index2, bloom_filter, sst2_reader, output_writer);
    }

    int num_pages_written_to_file = output_writer->WriteEndOfFile();
    // We now have the exact number of pages of data that we wrote
    // to the B-tree's leaf level, so update the file's data size.
    sort_merged_file->SetFileDataSize(num_pages_written_to_file * SST::KV_PAIRS_PER_PAGE * SST::KV_PAIR_BYTE_SIZE);

    // Close files
    close(fd1);
    close(fd2);

    // Empty this level.
    Level::DeleteSSTFiles();
}

void Level::AddSSTFile(SST *sst_file) { this->sst_files.push_back(sst_file); }

std::vector<SST *> Level::GetSSTFiles() { return this->sst_files; }

void Level::DeleteSSTFiles() {
    for (auto sst_file : this->sst_files) {
        std::filesystem::remove(sst_file->GetFileName());
    }
    this->sst_files = {};
}
