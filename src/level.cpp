#include <cmath>
#include <filesystem>
#include <iostream>
#include <unistd.h>
#include "Level.h"


Level::Level(int level, int bloomFilterBitsPerEntry, int inputBufferCapacity, int outputBufferCapacity) {
    this->level = level;
    this->bloomFilterBitsPerEntry = bloomFilterBitsPerEntry;
    this->sstFiles = {};
    this->inputBufferCapacity = inputBufferCapacity;
    this->outputBufferCapacity = outputBufferCapacity;
}

Level::~Level() {
    for (auto sstFile: this->sstFiles) {
        delete sstFile;
    }
}

int Level::GetLevelNumber() const {
    return this->level;
}

void Level::WriteDataToLevel(std::vector<DataEntry_t> data, SearchType searchType, std::string &dbPath) {
    std::string fileName = Utils::GetFilenameWithExt(std::to_string(this->sstFiles.size()));
    std::string filePath = dbPath + "/" + Utils::LEVEL + std::to_string(this->level) + "-" + fileName;
    uint64_t dataByteSize = data.size() * SST::KV_PAIR_BYTE_SIZE;
    auto *bloomFilter = new BloomFilter(this->bloomFilterBitsPerEntry, data.size());
    bloomFilter->InsertKeys(data);
    SST *sstFile = new SST(filePath, dataByteSize, bloomFilter);
    sstFile->SetupBTreeFile();
    sstFile->SetInputReader(new InputReader(sstFile->GetMaxOffsetToReadLeaves(), this->inputBufferCapacity));
    sstFile->SetScanInputReader(new ScanInputReader(this->inputBufferCapacity));
    std::ofstream file(sstFile->GetFileName(), std::ios::out | std::ios::binary);
    sstFile->WriteFile(file, data, searchType, true);
    this->sstFiles.push_back(sstFile);
}

void WriteRemainingData(int fd, int index, BloomFilter *bloomFilter, InputReader *reader, OutputWriter *outputWriter) {
    while (index < reader->GetInputBufferSize()) {
        DataEntry_t entry = reader->GetEntry(index);
        outputWriter->AddToOutputBuffer(entry);
        bloomFilter->InsertKey(entry.first);
        index += 2;
        // In the edge case when sst1Reader has greater keys than the sst2Reader,
        // make sure you read all the contents of sst1Reader.
        if (index >= reader->GetInputBufferSize()) {
            reader->ReadDataPagesInBuffer(fd);
            index = 0;
        }
    }
}

void Level::SortMergeAndWriteToNextLevel(Level *nextLevel, std::string &dbPath) {
    uint64_t sstDataSize = 0;
    for (auto sstFile: this->sstFiles) {
        sstDataSize += sstFile->GetFileDataSize();
    }

    // Create and setup new SST file for sort-merged data
    std::string fileName = Utils::GetFilenameWithExt(std::to_string(nextLevel->sstFiles.size()));
    std::string filePath = dbPath + "/" + Utils::LEVEL + std::to_string(nextLevel->level) + "-" + fileName;
    int maxNumKeys = std::ceil(sstDataSize / SST::KV_PAIR_BYTE_SIZE);
    auto *bloomFilter = new BloomFilter(this->bloomFilterBitsPerEntry, maxNumKeys);
    SST *sortMergedFile = new SST(filePath, sstDataSize, bloomFilter);

    sortMergedFile->SetupBTreeFile();
    sortMergedFile->SetInputReader(
            new InputReader(sortMergedFile->GetMaxOffsetToReadLeaves(), this->inputBufferCapacity));
    sortMergedFile->SetScanInputReader(new ScanInputReader(this->inputBufferCapacity));
    nextLevel->AddSSTFile(sortMergedFile);

    // Sort-merge data
    int fd1 = Utils::OpenFile(this->sstFiles[0]->GetFileName());
    if (fd1 == -1) {
        return;
    }

    int fd2 = Utils::OpenFile(this->sstFiles[1]->GetFileName());
    if (fd2 == -1) {
        return;
    }

    // InputReader will read 1 page of data from files at a time
    InputReader *sst1Reader = this->sstFiles[0]->GetInputReader();
    sst1Reader->ObtainOffsetToRead(fd1);
    InputReader *sst2Reader = this->sstFiles[1]->GetInputReader();
    sst2Reader->ObtainOffsetToRead(fd2);

    // Consider the output buffer size to be this->bufferCapacity page
    auto *outputWriter = new OutputWriter(sortMergedFile, this->outputBufferCapacity);

    sst1Reader->ReadDataPagesInBuffer(fd1);
    sst2Reader->ReadDataPagesInBuffer(fd2);
    int index1 = 0;
    int index2 = 0;
    while (sst1Reader->GetInputBufferSize() && sst2Reader->GetInputBufferSize()) {
        DataEntry_t entry1 = sst1Reader->GetEntry(index1);
        DataEntry_t entry2 = sst2Reader->GetEntry(index2);
        if (entry1.first < entry2.first) {
            outputWriter->AddToOutputBuffer(entry1);
            bloomFilter->InsertKey(entry1.first);
            index1 += 2;
        } else if (entry2.first < entry1.first) {
            outputWriter->AddToOutputBuffer(entry2);
            bloomFilter->InsertKey(entry2.first);
            index2 += 2;
        } else {
            // It is an update/delete
            outputWriter->AddToOutputBuffer(entry2);
            bloomFilter->InsertKey(entry2.first);
            index1 += 2;
            index2 += 2;
        }

        if (index1 >= sst1Reader->GetInputBufferSize()) {
            sst1Reader->ReadDataPagesInBuffer(fd1);
            index1 = 0;
        }
        if (index2 >= sst2Reader->GetInputBufferSize()) {
            sst2Reader->ReadDataPagesInBuffer(fd2);
            index2 = 0;
        }
    }

    // Write all the elements of the dataBuffer that still has remaining data to the output buffer
    if (sst1Reader->GetInputBufferSize()) {
        WriteRemainingData(fd1, index1, bloomFilter, sst1Reader, outputWriter);
    } else if (sst2Reader->GetInputBufferSize()) {
        WriteRemainingData(fd2, index2, bloomFilter, sst2Reader, outputWriter);
    }

    int numPagesWrittenToFile = outputWriter->WriteEndOfFile();
    // We now have the exact number of pages of data that we wrote
    // to the B-tree's leaf level, so update the file's data size.
    sortMergedFile->SetFileDataSize(numPagesWrittenToFile * SST::KV_PAIRS_PER_PAGE * SST::KV_PAIR_BYTE_SIZE);

    // Close files
    close(fd1);
    close(fd2);

    // Empty this level.
    Level::DeleteSSTFiles();
}

void Level::AddSSTFile(SST *sstFile) {
    this->sstFiles.push_back(sstFile);
}

std::vector<SST *> Level::GetSSTFiles() {
    return this->sstFiles;
}

void Level::DeleteSSTFiles() {
    for (auto sstFile: this->sstFiles) {
        std::filesystem::remove(sstFile->GetFileName());
    }
    this->sstFiles = {};
}