#include "sst.h"

#include <unistd.h>

#include <cmath>
#include <iostream>
#include <limits>
#include <list>
#include <queue>
#include <set>

SST::SST(std::string &fileName, uint64_t fileDataByteSize, BloomFilter *bloomFilter) {
    this->fileName = fileName;
    this->fileDataByteSize = fileDataByteSize;
    this->bloomFilter = bloomFilter;
    this->bTreeLevels = {};
    this->maxOffsetToReadLeaves = 0;
    this->inputReader = nullptr;
    this->scanInputReader = nullptr;
}

std::string SST::GetFileName() { return this->fileName; }

uint64_t SST::GetFileDataSize() const { return this->fileDataByteSize; }

void SST::SetFileDataSize(uint64_t newFileDataByteSize) { this->fileDataByteSize = newFileDataByteSize; }

InputReader *SST::GetInputReader() { return this->inputReader; }

void SST::SetInputReader(InputReader *newInputReader) { this->inputReader = newInputReader; }

uint64_t SST::GetMaxOffsetToReadLeaves() const { return this->maxOffsetToReadLeaves; }

ScanInputReader *SST::GetScanInputReader() { return this->scanInputReader; }

void SST::SetScanInputReader(ScanInputReader *newScanInputReader) { this->scanInputReader = newScanInputReader; }

std::vector<uint64_t> SST::GetBTreeLevelOffsets(int leavesNumPages) {
    // B = SST::KEYS_PER_PAGE
    std::vector<uint64_t> levelsSizes;
    uint64_t numPagesInLevel = leavesNumPages;
    levelsSizes.push_back(numPagesInLevel);
    while (numPagesInLevel > 1) {
        numPagesInLevel = std::ceil(numPagesInLevel / (double)SST::KEYS_PER_PAGE);
        levelsSizes.push_back(numPagesInLevel);
    }

    std::vector<uint64_t> levelOffsets;
    uint64_t numOfNodesSoFar = 1;
    levelOffsets.push_back(numOfNodesSoFar);
    for (int i = levelsSizes.size() - 1; i > 0; i--) {
        numOfNodesSoFar += levelsSizes[i];
        levelOffsets.push_back(numOfNodesSoFar);
    }
    this->maxOffsetToReadLeaves = numOfNodesSoFar + levelsSizes[0] - 1;
    return levelOffsets;
}

void SST::SetupBTreeFile() {
    int leavesNumPages = std::ceil(this->GetFileDataSize() / (double)SST::PAGE_SIZE);
    std::vector<uint64_t> levelOffsets = this->GetBTreeLevelOffsets(leavesNumPages);

    // Make B-tree levels
    for (uint64_t levelOffset : levelOffsets) {
        auto *level = new BTreeLevel(levelOffset * SST::PAGE_SIZE);
        this->bTreeLevels.push_back(level);
    }
}

void SST::WriteEndOfBTreeFile(std::ofstream &file) {
    // Write any non-complete internal nodes to the file
    this->WriteBTreeInternalLevels(file, true);

    // Write the bloom filter.
    if (this->bloomFilter != nullptr) {
        SST::WriteBloomFilter(file);
    }
    this->WriteBTreeMetaData(file);

    // We do not need the bloom filter's array anymore,
    // since from now on, we will read it from the static sst file.
    if (this->bloomFilter != nullptr) {
        this->bloomFilter->ClearFilterArray();
    }
    file.close();
}

void SST::WriteFile(std::ofstream &file, std::vector<DataEntry_t> &data, SearchType searchType, bool endOfFile) {
    if (searchType == SearchType::BINARY_SEARCH) {
        for (DataEntry_t &i : data) {
            file.write(reinterpret_cast<const char *>(&i.first), sizeof(uint64_t));
            file.write(reinterpret_cast<const char *>(&i.second), sizeof(uint64_t));
        }
        // Mark the last valid value of a page by an invalidValue, if the data is not aligned.
        if (data.size() % SST::KV_PAIRS_PER_PAGE != 0) {
            SST::WriteExtraToAlign(file, 1);
        }
        file.close();
        return;
    }

    if (searchType == SearchType::B_TREE_SEARCH && !data.empty()) {
        // B_TREE_SEARCH
        // The output file will be:
        /* Page 1 of the file: */
        /* Number of B-tree levels | page index where level 1 starts | page index where level 2 starts | ...    | */
        /* Next pages of the file: */
        /* | level 0 (i.e. root node) | level 1 | ... | level 2 | ... |
        Number of pages of the bloomFilter | Page where the bloomFilter starts| */
        this->WriteBTreeLevels(file, data, endOfFile);
    }

    // Write 1 page of metadata
    if (endOfFile) {
        this->WriteEndOfBTreeFile(file);
    }
}

void SST::WriteExtraToAlign(std::ofstream &file, uint64_t extraSpace) {
    uint64_t invalidValue = Utils::INVALID_VALUE;
    for (int i = 0; i < extraSpace; i++) {
        file.write(reinterpret_cast<const char *>(&invalidValue), sizeof(uint64_t));
    }
}

void SST::WriteBTreeMetaData(std::ofstream &file) {
    uint64_t numLevels = this->bTreeLevels.size();

    // MetaData will be the first page
    file.seekp(0, std::ios_base::beg);
    file.write(reinterpret_cast<const char *>(&numLevels), sizeof(uint64_t));

    // Write the BTree's metadata.
    for (uint64_t i = 0; i < numLevels; i++) {
        uint64_t levelStartOffset = std::ceil(this->bTreeLevels[i]->GetStartingByteOffset() / (double)SST::PAGE_SIZE);
        file.write(reinterpret_cast<const char *>(&levelStartOffset), sizeof(uint64_t));
    }

    // Write the bloom filter's metadata.
    if (this->bloomFilter != nullptr) {
        uint64_t bloomfilterNumPages = std::ceil(this->bloomFilter->GetFilterArraySize() / (double)SST::KEYS_PER_PAGE);
        file.write(reinterpret_cast<const char *>(&bloomfilterNumPages), sizeof(uint64_t));

        // Write where the bloom filter starts.
        uint64_t nextByteOffsetToWrite = this->bTreeLevels[numLevels - 1]->GetNextByteOffsetToWrite();
        uint64_t bloomFilterStartPage = std::ceil(nextByteOffsetToWrite / (double)SST::PAGE_SIZE);
        file.write(reinterpret_cast<const char *>(&bloomFilterStartPage), sizeof(uint64_t));
    }
    SST::WriteExtraToAlign(file, 1);
}

void SST::AddNextInternalLevelFenceKeys(std::vector<uint64_t> &data, int nextLevel) {
    int numInternalNodes = std::ceil(data.size() / (double)SST::KEYS_PER_PAGE);
    for (int i = 1; i <= numInternalNodes; i++) {
        size_t lastIndex = (i * SST::KEYS_PER_PAGE) - 1;
        if (lastIndex >= data.size()) {
            lastIndex = data.size() - 1;
        }

        // Put the fence keys in the next level
        this->bTreeLevels[nextLevel]->AddDataToLevel(data[lastIndex]);
    }
}

void SST::WriteBTreeInternalLevels(std::ofstream &file, bool endOfFile) {
    // Write internal levels
    for (int i = this->bTreeLevels.size() - 2; i >= 0; i--) {
        std::vector<uint64_t> levelData = this->bTreeLevels[i]->GetLevelData();
        bool levelIsAtLeastOneFullPage = (levelData.size() / SST::KEYS_PER_PAGE) >= 1;
        if (i > 0 && (levelIsAtLeastOneFullPage || endOfFile)) {
            this->AddNextInternalLevelFenceKeys(levelData, i - 1);
        }

        if (levelIsAtLeastOneFullPage || endOfFile) {
            file.seekp(this->bTreeLevels[i]->GetNextByteOffsetToWrite(), std::ios_base::beg);
            uint64_t *dataArray = levelData.data();
            file.write(reinterpret_cast<const char *>(&dataArray[0]), sizeof(uint64_t) * levelData.size());
            this->bTreeLevels[i]->IncrementNextByteOffsetToWrite(levelData.size() * SST::KEY_BYTE_SIZE);
            if (levelData.size() % SST::KEYS_PER_PAGE) {
                SST::WriteExtraToAlign(file, 1);
            }
            this->bTreeLevels[i]->ClearLevel();
        }
    }
}

void SST::WriteBTreeLevels(std::ofstream &file, std::vector<DataEntry_t> &data, bool endOfFile) {
    uint64_t numLevels = this->bTreeLevels.size();
    int numLeaves = std::ceil(data.size() / (double)SST::KV_PAIRS_PER_PAGE);
    if (numLevels > 1) {
        for (int i = 1; i <= numLeaves; i++) {
            size_t lastPairIndex = (i * SST::KV_PAIRS_PER_PAGE) - 1;
            if (lastPairIndex >= data.size()) {
                lastPairIndex = data.size() - 1;
            }
            // Put the fence keys of the leaves in the next internal level
            this->bTreeLevels[numLevels - 2]->AddDataToLevel(data[lastPairIndex].first);
        }
    }

    // Write the leaves by seeking to the beginning of where the leaves level starts
    uint64_t leavesOffsetToWrite = this->bTreeLevels[numLevels - 1]->GetNextByteOffsetToWrite();
    file.seekp(leavesOffsetToWrite, std::ios_base::beg);
    for (DataEntry_t &i : data) {
        file.write(reinterpret_cast<const char *>(&i.first), sizeof(uint64_t));
        file.write(reinterpret_cast<const char *>(&i.second), sizeof(uint64_t));
    }
    this->bTreeLevels[numLevels - 1]->IncrementNextByteOffsetToWrite(data.size() * SST::KV_PAIR_BYTE_SIZE);

    if (endOfFile) {
        uint64_t nextByteOffsetToWrite = this->bTreeLevels[numLevels - 1]->GetNextByteOffsetToWrite();
        this->maxOffsetToReadLeaves = std::ceil(nextByteOffsetToWrite / (double)SST::PAGE_SIZE) - 1;

        // Mark the last valid value of a page by an invalidValue, if the data is not page-aligned.
        if (data.size() % KV_PAIRS_PER_PAGE) {
            SST::WriteExtraToAlign(file, 1);
        }
    }
    this->bTreeLevels[numLevels - 1]->ClearLevel();
    this->WriteBTreeInternalLevels(file, false);
}

void SST::WriteBloomFilter(std::ofstream &file) {
    uint64_t offsetToWrite = (this->GetMaxOffsetToReadLeaves() + 1) * SST::PAGE_SIZE;
    file.seekp(offsetToWrite, std::ios_base::beg);

    std::vector<uint64_t> bloomFilterArray = this->bloomFilter->GetFilterArray();
    uint64_t size = sizeof(uint64_t) * this->bloomFilter->GetFilterArraySize();
    file.write(reinterpret_cast<const char *>(&bloomFilterArray[0]), size);
}

std::vector<uint64_t> SST::ReadPagesOfFile(int fd, uint64_t offset, uint64_t numPagesToRead) {
    uint64_t buffer[numPagesToRead * SST::KEYS_PER_PAGE];
    ssize_t bytesRead = pread(fd, buffer, numPagesToRead * SST::PAGE_SIZE, offset * SST::PAGE_SIZE);
    if (bytesRead == -1) {
        perror("pread");
    }

    std::vector<uint64_t> keys;
    if (bytesRead == 0) {
        return keys;
    }

    for (int i = 0; i < numPagesToRead * KEYS_PER_PAGE; i++) {
        // Reached the end of the data
        if (buffer[i] == Utils::INVALID_VALUE) {
            break;
        }
        keys.push_back(buffer[i]);
    }
    return keys;
}

std::vector<uint64_t> SST::ReadBTreeLevelOffsets(int fd) {
    std::vector<uint64_t> metadata = SST::ReadPagesOfFile(fd, 0);
    if (metadata.empty()) {
        return metadata;
    }

    // BTree's metadata starts at page index 0
    uint64_t numOfLevels = metadata[0];
    std::vector<uint64_t> levelsPageOffsets;
    for (int i = 1; i <= numOfLevels; i++) {
        levelsPageOffsets.push_back(metadata[i]);
    }
    return levelsPageOffsets;
}

std::vector<uint64_t> SST::ReadBloomFilter(int fd, uint64_t offset, uint64_t numPagesToRead) {
    auto *buffer = new uint64_t[numPagesToRead * SST::KEYS_PER_PAGE];
    ssize_t bytesRead = pread(fd, buffer, numPagesToRead * SST::PAGE_SIZE, offset * SST::PAGE_SIZE);
    if (bytesRead == -1) {
        perror("pread");
    }

    uint64_t numElements = numPagesToRead * SST::KEYS_PER_PAGE;
    std::vector<uint64_t> data((uint64_t *)buffer, (uint64_t *)buffer + numElements);
    delete[] buffer;
    return data;
}

std::vector<uint64_t> SST::GetPage(const std::string &pageId, int fd, uint64_t offset, BufferPool *bufferPool) {
    std::vector<uint64_t> data;
    if (bufferPool != nullptr) {
        data = bufferPool->Get(pageId);
    }

    // Read one page of the file if page not in buffer pool
    if (data.empty()) {
        data = SST::ReadPagesOfFile(fd, offset);
        // Save this page into the buffer pool
        if (bufferPool != nullptr) {
            bufferPool->Insert(pageId, data);
        }
    }
    return data;
}

std::vector<uint64_t> SST::GetBloomFilterPages(const std::string &pageId, int fd, uint64_t offset, uint64_t numPages,
                                               BufferPool *bufferPool) {
    if (bufferPool != nullptr) {
        std::vector<uint64_t> data = bufferPool->Get(pageId);
        if (!data.empty()) {
            return data;
        }
    }

    // Read the bloom filter array if it was not in buffer pool.
    std::vector<uint64_t> data = SST::ReadBloomFilter(fd, offset, numPages);

    // Save the bloom filter in the buffer pool
    if (bufferPool != nullptr) {
        bufferPool->Insert(pageId, data);
    }
    return data;
}

uint64_t SST::PerformBinarySearch(uint64_t key, BufferPool *bufferPool) {
    int fd = Utils::OpenFile(this->fileName);
    if (fd == -1) {
        return Utils::INVALID_VALUE;
    }

    int numPages = ceil(this->fileDataByteSize / SST::PAGE_SIZE);

    // Read the file and do a binary search on that to look for the key
    uint64_t value = Utils::INVALID_VALUE;
    int start = 0;
    int end = (numPages == 0) ? 0 : numPages - 1;
    int offsetToRead = start;
    while (start <= end) {
        // Make sure we read the 1st and last pages of the file first to see
        // if the key can ever be potentially in this file. If not, we end the search sooner.
        if (start > 0 && end < numPages - 1) {
            offsetToRead = start + (end - start) / 2;
        } else if (start > 0 && end == numPages - 1) {
            offsetToRead = end;
        }

        // See if the buffer pool has this page, else
        // read this page and insert it into the buffer pool.
        std::string pageId = this->GetPageIdInBufferPool(offsetToRead);
        std::vector<uint64_t> data = SST::GetPage(pageId, fd, offsetToRead, bufferPool);
        if (data.empty()) {  // No more data in SST, break out of the loop
            break;
        }

        // Get the keys from the data.
        std::vector<uint64_t> keys = Utils::GetKeys(data);
        if (key < keys[0]) {
            end = offsetToRead - 1;
        } else if (key > keys[keys.size() - 1]) {
            start = offsetToRead + 1;
        } else {
            int index = Utils::BinarySearch(keys, key);
            // Found the data, break out of the loop
            if (index < keys.size() && keys[index] == key) {
                value = data[index * 2 + 1];  // values are in odd indexes
            }
            break;
        }
    }
    close(fd);
    return value;
}

uint64_t SST::FindKeyInBTree(int fd, uint64_t key, BufferPool *bufferPool, std::vector<uint64_t> &levelsPageOffsets) {
    uint64_t value = Utils::INVALID_VALUE;
    uint64_t numOfLevels = levelsPageOffsets.size();
    uint64_t currLevel = 0;
    uint64_t offsetToRead = levelsPageOffsets[0];  // Should be 1 always
    uint64_t prevLevelChildIndex = 0;
    int index;
    while (currLevel < numOfLevels) {
        // See if the buffer pool has this page, else
        // read this page and insert it into the buffer pool.
        std::string pageId = this->GetPageIdInBufferPool(offsetToRead);
        std::vector<uint64_t> data = SST::GetPage(pageId, fd, offsetToRead, bufferPool);
        if (data.empty()) {
            return value;
        }

        if (currLevel == numOfLevels - 1) {
            // This is the leaves level. Get the keys from the data.
            std::vector<uint64_t> keys = Utils::GetKeys(data);

            index = Utils::BinarySearch(keys, key);
            if (index < keys.size() && keys[index] == key) {
                value = data[index * 2 + 1];  // values are in odd indexes
            }
            return value;
        } else {
            index = Utils::BinarySearch(data, key);
        }

        // Key not found
        if (index >= data.size()) {
            return Utils::INVALID_VALUE;
        }
        offsetToRead = levelsPageOffsets[currLevel + 1];
        offsetToRead += prevLevelChildIndex * SST::KEYS_PER_PAGE + index;
        prevLevelChildIndex = index;
        currLevel++;
    }

    // Key not found
    return value;
}

uint64_t SST::PerformBTreeSearch(uint64_t key, BufferPool *bufferPool, bool isLSMTree) {
    uint64_t value = Utils::INVALID_VALUE;
    int fd = Utils::OpenFile(this->fileName);
    if (fd == -1) {
        return value;
    }

    uint64_t offsetToRead = 0;
    // See if the buffer pool has this page, else
    // read this page and insert it into the buffer pool.
    std::string metadataPageId = this->GetPageIdInBufferPool(offsetToRead);
    std::vector<uint64_t> metadata = SST::GetPage(metadataPageId, fd, offsetToRead, bufferPool);
    if (metadata.empty()) {
        close(fd);
        return value;
    }

    uint64_t numOfLevels = metadata[0];
    // Check the bloom filter if one is defined for this sst file.
    if (isLSMTree) {
        uint64_t numPagesToRead = metadata[numOfLevels + 1];
        offsetToRead = metadata[numOfLevels + 2];  // offset where the bloom filter starts
        std::string pageId = this->GetPageIdInBufferPool(offsetToRead);
        auto bloomFilterArray = SST::GetBloomFilterPages(pageId, fd, offsetToRead, numPagesToRead, bufferPool);
        if (this->bloomFilter && !this->bloomFilter->KeyProbablyExists(key, bloomFilterArray)) {
            close(fd);
            return value;
        }
    }

    std::vector<uint64_t> levelsPageOffsets;
    for (int i = 1; i <= numOfLevels; i++) {
        // When computing the index of the starting page of each level,
        // count for this page by adding a one.
        levelsPageOffsets.push_back(metadata[i]);
    }

    uint64_t result = SST::FindKeyInBTree(fd, key, bufferPool, levelsPageOffsets);
    close(fd);
    return result;
}

void SST::PerformBinaryScan(uint64_t key1, uint64_t key2, std::vector<DataEntry_t> &scanResult) {
    int fd = Utils::OpenFile(this->fileName);
    if (fd == -1) {
        return;
    }

    // Read the file and do a binary search on that to look for the key1
    bool foundKey1 = false;
    int numOfPagesOfFile = ceil((double)this->fileDataByteSize / SST::PAGE_SIZE);
    int start = 0;
    int end = numOfPagesOfFile - 1;
    int offsetToRead;
    std::set<uint64_t> pagesReadSoFar;
    uint64_t key1Page = 0;
    while (!foundKey1 && start <= end) {
        // Perform a binary search for key1
        offsetToRead = start + (end - start) / 2;
        std::vector<uint64_t> data;
        data = SST::ReadPagesOfFile(fd, offsetToRead);
        pagesReadSoFar.insert(offsetToRead);

        // Get the keys from the data.
        std::vector<uint64_t> keys = Utils::GetKeys(data);

        uint64_t startIndex = 0;
        uint64_t endIndex = keys.size() - 1;

        if (key1 < keys[0]) {
            end = offsetToRead - 1;
        } else if (key1 > keys[keys.size() - 1]) {
            start = offsetToRead + 1;
            // Skip this page
            startIndex = std::numeric_limits<int>::max();  // max value
            if (offsetToRead == numOfPagesOfFile - 1) {
                close(fd);
                return;
            }
        } else {
            int index = Utils::BinarySearch(keys, key1);
            foundKey1 = true;
            key1Page = offsetToRead;
            startIndex = index >= keys.size() ? keys.size() - 1 : index;
        }

        if (key2 <= keys[keys.size() - 1]) {
            int index = Utils::BinarySearch(keys, key2);
            endIndex = index >= keys.size() ? keys.size() - 1 : index;
        }

        // Read this page if you have not skipped it.
        for (uint64_t i = startIndex; i <= endIndex; i++) {
            // values are in odd indexes, so get them at i * 2 + 1.
            scanResult.emplace_back(keys[i], data[i * 2 + 1]);
        }
    }

    // If the key1 is less than the min key in the file, we will not find it
    // in the loop above. At this step, we know that key1Page should be 0.
    if (!foundKey1) {
        key1Page = 0;
    }

    // Read pages that have not been read starting from one page after we found key1.
    bool foundKey2 = false;
    uint64_t nextPageToRead = key1Page + 1;
    while (!foundKey2 && nextPageToRead < numOfPagesOfFile) {
        if (!pagesReadSoFar.count(nextPageToRead)) {
            std::vector<uint64_t> data = SST::ReadPagesOfFile(fd, nextPageToRead);
            for (int i = 0; i < data.size(); i += 2) {
                // Read until we find a key greater than key2.
                if (data[i] > key2) {
                    foundKey2 = true;
                    break;
                }
                // values are in odd indexes, so get them at i + 1.
                scanResult.emplace_back(data[i], data[i + 1]);
            }
        }
        nextPageToRead++;
    }
    close(fd);
}

uint64_t SST::ReadBTreeScanLeavesRange(int fd, uint64_t key1) {
    // Read one page of the file containing the metadata of the tree.
    std::vector<uint64_t> levelsPageOffsets = SST::ReadBTreeLevelOffsets(fd);
    int currLevel = 0;
    uint64_t offsetToRead = levelsPageOffsets[0];
    uint64_t numOfLevels = levelsPageOffsets.size();

    // A queue of pairs of page offset to read and its index at its parent node.
    std::queue<DataEntry_t> queue;
    queue.emplace(offsetToRead, 0);
    while (currLevel != numOfLevels - 1) {
        std::vector<DataEntry_t> keysIndexesLimits;
        std::vector<DataEntry_t> keysIndexes;
        DataEntry_t curPageToRead = queue.front();
        queue.pop();
        std::vector<uint64_t> data = SST::ReadPagesOfFile(fd, curPageToRead.first);
        if (data.empty()) {
            break;
        }

        uint64_t keyCeilIndex = Utils::BinarySearch(data, key1);
        offsetToRead = levelsPageOffsets[currLevel + 1];
        offsetToRead += curPageToRead.second * SST::KEYS_PER_PAGE + keyCeilIndex;
        if (keyCeilIndex >= data.size()) {
            keyCeilIndex = data.size() - 1;
        }
        queue.emplace(offsetToRead, keyCeilIndex);
        currLevel++;
    }
    return queue.front().first;
}

void SST::PerformBTreeScan(uint64_t key1, uint64_t key2, std::vector<DataEntry_t> &scanResult) {
    int fd = Utils::OpenFile(this->fileName);
    if (fd == -1) {
        return;
    }

    uint64_t offsetToRead = SST::ReadBTreeScanLeavesRange(fd, key1);
    // Read all the pages between offsetToRead (where the key1 is) and
    // this->maxOffsetToReadLeaves, until you either find key2 or reach end of the leaves.
    while (offsetToRead <= this->maxOffsetToReadLeaves) {
        std::vector<uint64_t> data = SST::ReadPagesOfFile(fd, offsetToRead);
        for (int i = 0; i < SST::KEYS_PER_PAGE - 1; i += 2) {
            if (data[i] > key2 || data[i] == Utils::INVALID_VALUE) {
                break;
            } else if (data[i] >= key1) {
                scanResult.emplace_back(data[i], data[i + 1]);
            }
        }
        offsetToRead++;
    }
    close(fd);
}
