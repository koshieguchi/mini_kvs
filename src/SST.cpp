#include "sst.h"

#include <unistd.h>

#include <cmath>
#include <iostream>
#include <limits>
#include <list>
#include <queue>
#include <set>

SST::SST(std::string &file_name, uint64_t file_data_byte_size, BloomFilter *bloom_filter) {
    this->file_name = file_name;
    this->file_data_byte_size = file_data_byte_size;
    this->bloom_filter = bloom_filter;
    this->b_tree_levels = {};
    this->max_offset_to_readLeaves = 0;
    this->input_reader = nullptr;
    this->scan_input_reader = nullptr;
}

std::string SST::GetFileName() { return this->file_name; }

uint64_t SST::GetFileDataSize() const { return this->file_data_byte_size; }

void SST::SetFileDataSize(uint64_t new_file_data_byte_size) { this->file_data_byte_size = new_file_data_byte_size; }

InputReader *SST::GetInputReader() { return this->input_reader; }

void SST::SetInputReader(InputReader *new_input_reader) { this->input_reader = new_input_reader; }

uint64_t SST::GetMaxOffsetToReadLeaves() const { return this->max_offset_to_readLeaves; }

ScanInputReader *SST::GetScanInputReader() { return this->scan_input_reader; }

void SST::SetScanInputReader(ScanInputReader *new_scan_input_reader) {
    this->scan_input_reader = new_scan_input_reader;
}

std::vector<uint64_t> SST::GetBTreeLevelOffsets(int leaves_num_pages) {
    // B = SST::KEYS_PER_PAGE
    std::vector<uint64_t> levels_sizes;
    uint64_t num_pagesInLevel = leaves_num_pages;
    levels_sizes.push_back(num_pagesInLevel);
    while (num_pagesInLevel > 1) {
        num_pagesInLevel = std::ceil(num_pagesInLevel / (double)SST::KEYS_PER_PAGE);
        levels_sizes.push_back(num_pagesInLevel);
    }

    std::vector<uint64_t> level_offsets;
    uint64_t num_of_nodes_so_far = 1;
    level_offsets.push_back(num_of_nodes_so_far);
    for (int i = levels_sizes.size() - 1; i > 0; i--) {
        num_of_nodes_so_far += levels_sizes[i];
        level_offsets.push_back(num_of_nodes_so_far);
    }
    this->max_offset_to_readLeaves = num_of_nodes_so_far + levels_sizes[0] - 1;
    return level_offsets;
}

void SST::SetupBTreeFile() {
    int leaves_num_pages = std::ceil(this->GetFileDataSize() / (double)SST::PAGE_SIZE);
    std::vector<uint64_t> level_offsets = this->GetBTreeLevelOffsets(leaves_num_pages);

    // Make B-tree levels
    for (uint64_t levelOffset : level_offsets) {
        BTreeLevel *level = new BTreeLevel(levelOffset * SST::PAGE_SIZE);
        this->b_tree_levels.push_back(level);
    }
}

void SST::WriteEndOfBTreeFile(std::ofstream &file) {
    // Write any non-complete internal nodes to the file
    this->WriteBTreeInternalLevels(file, true);

    // Write the bloom filter.
    if (this->bloom_filter != nullptr) {
        SST::WriteBloomFilter(file);
    }
    this->WriteBTreeMetaData(file);

    // We do not need the bloom filter's array anymore,
    // since from now on, we will read it from the static sst file.
    if (this->bloom_filter != nullptr) {
        this->bloom_filter->ClearFilterArray();
    }
    file.close();
}

void SST::WriteFile(std::ofstream &file, std::vector<DataEntry_t> &data, SearchType search_type, bool end_of_file) {
    if (search_type == SearchType::BINARY_SEARCH) {
        for (DataEntry_t &i : data) {
            file.write(reinterpret_cast<const char *>(&i.first), sizeof(uint64_t));
            file.write(reinterpret_cast<const char *>(&i.second), sizeof(uint64_t));
        }
        // Mark the last valid value of a page by an invalid_value, if the data is not aligned.
        if (data.size() % SST::KV_PAIRS_PER_PAGE != 0) {
            SST::WriteExtraToAlign(file, 1);
        }
        file.close();
        return;
    }

    if (search_type == SearchType::B_TREE_SEARCH && !data.empty()) {
        // B_TREE_SEARCH
        // The output file will be:
        /* Page 1 of the file: */
        /* Number of B-tree levels | page index where level 1 starts | page index where level 2 starts | ...    | */
        /* Next pages of the file: */
        /* | level 0 (i.e. root node) | level 1 | ... | level 2 | ... |
        Number of pages of the bloom_filter | Page where the bloom_filter starts| */
        this->WriteBTreeLevels(file, data, end_of_file);
    }

    // Write 1 page of metadata
    if (end_of_file) {
        this->WriteEndOfBTreeFile(file);
    }
}

void SST::WriteExtraToAlign(std::ofstream &file, uint64_t extraSpace) {
    uint64_t invalid_value = Utils::INVALID_VALUE;
    for (int i = 0; i < extraSpace; i++) {
        file.write(reinterpret_cast<const char *>(&invalid_value), sizeof(uint64_t));
    }
}

void SST::WriteBTreeMetaData(std::ofstream &file) {
    uint64_t num_levels = this->b_tree_levels.size();

    // MetaData will be the first page
    file.seekp(0, std::ios_base::beg);
    file.write(reinterpret_cast<const char *>(&num_levels), sizeof(uint64_t));

    // Write the BTree's metadata.
    for (uint64_t i = 0; i < num_levels; i++) {
        uint64_t level_start_offset =
            std::ceil(this->b_tree_levels[i]->GetStartingByteOffset() / (double)SST::PAGE_SIZE);
        file.write(reinterpret_cast<const char *>(&level_start_offset), sizeof(uint64_t));
    }

    // Write the bloom filter's metadata.
    if (this->bloom_filter != nullptr) {
        uint64_t bloom_filter_num_pages =
            std::ceil(this->bloom_filter->GetFilterArraySize() / (double)SST::KEYS_PER_PAGE);
        file.write(reinterpret_cast<const char *>(&bloom_filter_num_pages), sizeof(uint64_t));

        // Write where the bloom filter starts.
        uint64_t next_byte_offset_to_write = this->b_tree_levels[num_levels - 1]->GetNextByteOffsetToWrite();
        uint64_t bloom_filter_start_page = std::ceil(next_byte_offset_to_write / (double)SST::PAGE_SIZE);
        file.write(reinterpret_cast<const char *>(&bloom_filter_start_page), sizeof(uint64_t));
    }
    SST::WriteExtraToAlign(file, 1);
}

void SST::AddNextInternalLevelFenceKeys(std::vector<uint64_t> &data, int next_level) {
    int num_internal_nodes = std::ceil(data.size() / (double)SST::KEYS_PER_PAGE);
    for (int i = 1; i <= num_internal_nodes; i++) {
        size_t last_index = (i * SST::KEYS_PER_PAGE) - 1;
        if (last_index >= data.size()) {
            last_index = data.size() - 1;
        }

        // Put the fence keys in the next level
        this->b_tree_levels[next_level]->AddDataToLevel(data[last_index]);
    }
}

void SST::WriteBTreeInternalLevels(std::ofstream &file, bool end_of_file) {
    // Write internal levels
    for (int i = this->b_tree_levels.size() - 2; i >= 0; i--) {
        std::vector<uint64_t> level_data = this->b_tree_levels[i]->GetLevelData();
        bool level_is_at_least_one_full_page = (level_data.size() / SST::KEYS_PER_PAGE) >= 1;
        if (i > 0 && (level_is_at_least_one_full_page || end_of_file)) {
            this->AddNextInternalLevelFenceKeys(level_data, i - 1);
        }

        if (level_is_at_least_one_full_page || end_of_file) {
            file.seekp(this->b_tree_levels[i]->GetNextByteOffsetToWrite(), std::ios_base::beg);
            uint64_t *dataArray = level_data.data();
            file.write(reinterpret_cast<const char *>(&dataArray[0]), sizeof(uint64_t) * level_data.size());
            this->b_tree_levels[i]->IncrementNextByteOffsetToWrite(level_data.size() * SST::KEY_BYTE_SIZE);
            if (level_data.size() % SST::KEYS_PER_PAGE) {
                SST::WriteExtraToAlign(file, 1);
            }
            this->b_tree_levels[i]->ClearLevel();
        }
    }
}

void SST::WriteBTreeLevels(std::ofstream &file, std::vector<DataEntry_t> &data, bool end_of_file) {
    uint64_t num_levels = this->b_tree_levels.size();
    int numLeaves = std::ceil(data.size() / (double)SST::KV_PAIRS_PER_PAGE);
    if (num_levels > 1) {
        for (int i = 1; i <= numLeaves; i++) {
            size_t last_pair_index = (i * SST::KV_PAIRS_PER_PAGE) - 1;
            if (last_pair_index >= data.size()) {
                last_pair_index = data.size() - 1;
            }
            // Put the fence keys of the leaves in the next internal level
            this->b_tree_levels[num_levels - 2]->AddDataToLevel(data[last_pair_index].first);
        }
    }

    // Write the leaves by seeking to the beginning of where the leaves level starts
    uint64_t leaves_offset_to_write = this->b_tree_levels[num_levels - 1]->GetNextByteOffsetToWrite();
    file.seekp(leaves_offset_to_write, std::ios_base::beg);
    for (DataEntry_t &i : data) {
        file.write(reinterpret_cast<const char *>(&i.first), sizeof(uint64_t));
        file.write(reinterpret_cast<const char *>(&i.second), sizeof(uint64_t));
    }
    this->b_tree_levels[num_levels - 1]->IncrementNextByteOffsetToWrite(data.size() * SST::KV_PAIR_BYTE_SIZE);

    if (end_of_file) {
        uint64_t next_byte_offset_to_write = this->b_tree_levels[num_levels - 1]->GetNextByteOffsetToWrite();
        this->max_offset_to_readLeaves = std::ceil(next_byte_offset_to_write / (double)SST::PAGE_SIZE) - 1;

        // Mark the last valid value of a page by an invalid_value, if the data is not page-aligned.
        if (data.size() % KV_PAIRS_PER_PAGE) {
            SST::WriteExtraToAlign(file, 1);
        }
    }
    this->b_tree_levels[num_levels - 1]->ClearLevel();
    this->WriteBTreeInternalLevels(file, false);
}

void SST::WriteBloomFilter(std::ofstream &file) {
    uint64_t offset_to_write = (this->GetMaxOffsetToReadLeaves() + 1) * SST::PAGE_SIZE;
    file.seekp(offset_to_write, std::ios_base::beg);

    std::vector<uint64_t> bloom_filter_array = this->bloom_filter->GetFilterArray();
    uint64_t size = sizeof(uint64_t) * this->bloom_filter->GetFilterArraySize();
    file.write(reinterpret_cast<const char *>(&bloom_filter_array[0]), size);
}

std::vector<uint64_t> SST::ReadPagesOfFile(int fd, uint64_t offset, uint64_t num_pages_to_read) {
    uint64_t buffer[num_pages_to_read * SST::KEYS_PER_PAGE];
    ssize_t bytes_read = pread(fd, buffer, num_pages_to_read * SST::PAGE_SIZE, offset * SST::PAGE_SIZE);
    if (bytes_read == -1) {
        perror("pread");
    }

    std::vector<uint64_t> keys;
    if (bytes_read == 0) {
        return keys;
    }

    for (int i = 0; i < num_pages_to_read * KEYS_PER_PAGE; i++) {
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
    std::vector<uint64_t> levels_page_offsets;
    for (int i = 1; i <= numOfLevels; i++) {
        levels_page_offsets.push_back(metadata[i]);
    }
    return levels_page_offsets;
}

std::vector<uint64_t> SST::ReadBloomFilter(int fd, uint64_t offset, uint64_t num_pages_to_read) {
    uint64_t *buffer = new uint64_t[num_pages_to_read * SST::KEYS_PER_PAGE];
    ssize_t bytes_read = pread(fd, buffer, num_pages_to_read * SST::PAGE_SIZE, offset * SST::PAGE_SIZE);
    if (bytes_read == -1) {
        perror("pread");
    }

    uint64_t numElements = num_pages_to_read * SST::KEYS_PER_PAGE;
    std::vector<uint64_t> data((uint64_t *)buffer, (uint64_t *)buffer + numElements);
    delete[] buffer;
    return data;
}

std::vector<uint64_t> SST::GetPage(const std::string &page_id, int fd, uint64_t offset, BufferPool *buffer_pool) {
    std::vector<uint64_t> data;
    if (buffer_pool != nullptr) {
        data = buffer_pool->Get(page_id);
    }

    // Read one page of the file if page not in buffer pool
    if (data.empty()) {
        data = SST::ReadPagesOfFile(fd, offset);
        // Save this page into the buffer pool
        if (buffer_pool != nullptr) {
            buffer_pool->Insert(page_id, data);
        }
    }
    return data;
}

std::vector<uint64_t> SST::GetBloomFilterPages(const std::string &page_id, int fd, uint64_t offset, uint64_t num_pages,
                                               BufferPool *buffer_pool) {
    if (buffer_pool != nullptr) {
        std::vector<uint64_t> data = buffer_pool->Get(page_id);
        if (!data.empty()) {
            return data;
        }
    }

    // Read the bloom filter array if it was not in buffer pool.
    std::vector<uint64_t> data = SST::ReadBloomFilter(fd, offset, num_pages);

    // Save the bloom filter in the buffer pool
    if (buffer_pool != nullptr) {
        buffer_pool->Insert(page_id, data);
    }
    return data;
}

uint64_t SST::PerformBinarySearch(uint64_t key, BufferPool *buffer_pool) {
    int fd = Utils::OpenFile(this->file_name);
    if (fd == -1) {
        return Utils::INVALID_VALUE;
    }

    int num_pages = ceil(this->file_data_byte_size / SST::PAGE_SIZE);

    // Read the file and do a binary search on that to look for the key
    uint64_t value = Utils::INVALID_VALUE;
    int start = 0;
    int end = (num_pages == 0) ? 0 : num_pages - 1;
    int offset_to_read = start;
    while (start <= end) {
        // Make sure we read the 1st and last pages of the file first to see
        // if the key can ever be potentially in this file. If not, we end the search sooner.
        if (start > 0 && end < num_pages - 1) {
            offset_to_read = start + (end - start) / 2;
        } else if (start > 0 && end == num_pages - 1) {
            offset_to_read = end;
        }

        // See if the buffer pool has this page, else
        // read this page and insert it into the buffer pool.
        std::string page_id = this->GetPageIdInBufferPool(offset_to_read);
        std::vector<uint64_t> data = SST::GetPage(page_id, fd, offset_to_read, buffer_pool);
        if (data.empty()) {  // No more data in SST, break out of the loop
            break;
        }

        // Get the keys from the data.
        std::vector<uint64_t> keys = Utils::GetKeys(data);
        if (key < keys[0]) {
            end = offset_to_read - 1;
        } else if (key > keys[keys.size() - 1]) {
            start = offset_to_read + 1;
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

uint64_t SST::FindKeyInBTree(int fd, uint64_t key, BufferPool *buffer_pool,
                             std::vector<uint64_t> &levels_page_offsets) {
    uint64_t value = Utils::INVALID_VALUE;
    uint64_t numOfLevels = levels_page_offsets.size();
    uint64_t cur_level = 0;
    uint64_t offset_to_read = levels_page_offsets[0];  // Should be 1 always
    uint64_t prevLevelChildIndex = 0;
    int index;
    while (cur_level < numOfLevels) {
        // See if the buffer pool has this page, else
        // read this page and insert it into the buffer pool.
        std::string page_id = this->GetPageIdInBufferPool(offset_to_read);
        std::vector<uint64_t> data = SST::GetPage(page_id, fd, offset_to_read, buffer_pool);
        if (data.empty()) {
            return value;
        }

        if (cur_level == numOfLevels - 1) {
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
        offset_to_read = levels_page_offsets[cur_level + 1];
        offset_to_read += prevLevelChildIndex * SST::KEYS_PER_PAGE + index;
        prevLevelChildIndex = index;
        cur_level++;
    }

    // Key not found
    return value;
}

uint64_t SST::PerformBTreeSearch(uint64_t key, BufferPool *buffer_pool, bool is_lsm_tree) {
    uint64_t value = Utils::INVALID_VALUE;
    int fd = Utils::OpenFile(this->file_name);
    if (fd == -1) {
        return value;
    }

    uint64_t offset_to_read = 0;
    // See if the buffer pool has this page, else
    // read this page and insert it into the buffer pool.
    std::string metadataPageId = this->GetPageIdInBufferPool(offset_to_read);
    std::vector<uint64_t> metadata = SST::GetPage(metadataPageId, fd, offset_to_read, buffer_pool);
    if (metadata.empty()) {
        close(fd);
        return value;
    }

    uint64_t numOfLevels = metadata[0];
    // Check the bloom filter if one is defined for this sst file.
    if (is_lsm_tree) {
        uint64_t num_pages_to_read = metadata[numOfLevels + 1];
        offset_to_read = metadata[numOfLevels + 2];  // offset where the bloom filter starts
        std::string page_id = this->GetPageIdInBufferPool(offset_to_read);
        auto bloom_filter_array = SST::GetBloomFilterPages(page_id, fd, offset_to_read, num_pages_to_read, buffer_pool);
        if (this->bloom_filter && !this->bloom_filter->KeyProbablyExists(key, bloom_filter_array)) {
            close(fd);
            return value;
        }
    }

    std::vector<uint64_t> levels_page_offsets;
    for (int i = 1; i <= numOfLevels; i++) {
        // When computing the index of the starting page of each level,
        // count for this page by adding a one.
        levels_page_offsets.push_back(metadata[i]);
    }

    uint64_t result = SST::FindKeyInBTree(fd, key, buffer_pool, levels_page_offsets);
    close(fd);
    return result;
}

void SST::PerformBinaryScan(uint64_t key1, uint64_t key2, std::vector<DataEntry_t> &scan_result) {
    int fd = Utils::OpenFile(this->file_name);
    if (fd == -1) {
        return;
    }

    // Read the file and do a binary search on that to look for the key1
    bool foundKey1 = false;
    int numOfPagesOfFile = ceil((double)this->file_data_byte_size / SST::PAGE_SIZE);
    int start = 0;
    int end = numOfPagesOfFile - 1;
    int offset_to_read;
    std::set<uint64_t> pagesReadSoFar;
    uint64_t key1Page = 0;
    while (!foundKey1 && start <= end) {
        // Perform a binary search for key1
        offset_to_read = start + (end - start) / 2;
        std::vector<uint64_t> data;
        data = SST::ReadPagesOfFile(fd, offset_to_read);
        pagesReadSoFar.insert(offset_to_read);

        // Get the keys from the data.
        std::vector<uint64_t> keys = Utils::GetKeys(data);

        uint64_t start_index = 0;
        uint64_t endIndex = keys.size() - 1;

        if (key1 < keys[0]) {
            end = offset_to_read - 1;
        } else if (key1 > keys[keys.size() - 1]) {
            start = offset_to_read + 1;
            // Skip this page
            start_index = std::numeric_limits<int>::max();  // max value
            if (offset_to_read == numOfPagesOfFile - 1) {
                close(fd);
                return;
            }
        } else {
            int index = Utils::BinarySearch(keys, key1);
            foundKey1 = true;
            key1Page = offset_to_read;
            start_index = index >= keys.size() ? keys.size() - 1 : index;
        }

        if (key2 <= keys[keys.size() - 1]) {
            int index = Utils::BinarySearch(keys, key2);
            endIndex = index >= keys.size() ? keys.size() - 1 : index;
        }

        // Read this page if you have not skipped it.
        for (uint64_t i = start_index; i <= endIndex; i++) {
            // values are in odd indexes, so get them at i * 2 + 1.
            scan_result.emplace_back(keys[i], data[i * 2 + 1]);
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
                scan_result.emplace_back(data[i], data[i + 1]);
            }
        }
        nextPageToRead++;
    }
    close(fd);
}

uint64_t SST::ReadBTreeScanLeavesRange(int fd, uint64_t key1) {
    // Read one page of the file containing the metadata of the tree.
    std::vector<uint64_t> levels_page_offsets = SST::ReadBTreeLevelOffsets(fd);
    int cur_level = 0;
    uint64_t offset_to_read = levels_page_offsets[0];
    uint64_t numOfLevels = levels_page_offsets.size();

    // A queue of pairs of page offset to read and its index at its parent node.
    std::queue<DataEntry_t> queue;
    queue.emplace(offset_to_read, 0);
    while (cur_level != numOfLevels - 1) {
        std::vector<DataEntry_t> keysIndexesLimits;
        std::vector<DataEntry_t> keysIndexes;
        DataEntry_t cur_pageToRead = queue.front();
        queue.pop();
        std::vector<uint64_t> data = SST::ReadPagesOfFile(fd, cur_pageToRead.first);
        if (data.empty()) {
            break;
        }

        uint64_t keyCeilIndex = Utils::BinarySearch(data, key1);
        offset_to_read = levels_page_offsets[cur_level + 1];
        offset_to_read += cur_pageToRead.second * SST::KEYS_PER_PAGE + keyCeilIndex;
        if (keyCeilIndex >= data.size()) {
            keyCeilIndex = data.size() - 1;
        }
        queue.emplace(offset_to_read, keyCeilIndex);
        cur_level++;
    }
    return queue.front().first;
}

void SST::PerformBTreeScan(uint64_t key1, uint64_t key2, std::vector<DataEntry_t> &scan_result) {
    int fd = Utils::OpenFile(this->file_name);
    if (fd == -1) {
        return;
    }

    uint64_t offset_to_read = SST::ReadBTreeScanLeavesRange(fd, key1);
    // Read all the pages between offset_to_read (where the key1 is) and
    // this->max_offset_to_readLeaves, until you either find key2 or reach end of the leaves.
    while (offset_to_read <= this->max_offset_to_readLeaves) {
        std::vector<uint64_t> data = SST::ReadPagesOfFile(fd, offset_to_read);
        for (int i = 0; i < SST::KEYS_PER_PAGE - 1; i += 2) {
            if (data[i] > key2 || data[i] == Utils::INVALID_VALUE) {
                break;
            } else if (data[i] >= key1) {
                scan_result.emplace_back(data[i], data[i + 1]);
            }
        }
        offset_to_read++;
    }
    close(fd);
}
