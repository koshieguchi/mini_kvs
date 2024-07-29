#include "lsm_tree.h"

#include <unistd.h>

#include <bitset>
#include <functional>
#include <queue>

LSMTree::LSMTree(int bloom_filter_bits_per_entry, int input_buffer_capacity, int output_buffer_capacity) {
    this->levels = {};
    this->bit_per_entry = bloom_filter_bits_per_entry;
    this->input_buffer_capacity = input_buffer_capacity;
    this->output_buffer_capacity = output_buffer_capacity;
}

LSMTree::~LSMTree() {
    for (auto level : this->levels) {
        delete level;
    }
}

void LSMTree::MaintainLevelCapacityAndCompact(Level *cur_level, std::string &kvs_path) {
    int level = cur_level->GetLevelNumber();
    if (this->levels[level]->GetSSTFiles().size() <= 1) {
        return;
    }

    if (level + 1 >= this->levels.size()) {
        auto *new_level =
            new Level(level + 1, this->bit_per_entry, this->input_buffer_capacity, this->output_buffer_capacity);
        this->levels.push_back(new_level);
    }

    Level *next_level = this->levels[level + 1];
    cur_level->SortMergeAndWriteToNextLevel(next_level, kvs_path);
    LSMTree::MaintainLevelCapacityAndCompact(next_level, kvs_path);
}

void LSMTree::WriteMemtableData(std::vector<DataEntry_t> &data, SearchType search_type, std::string &kvs_path) {
    // Always write the new sst files to the first level
    if (this->levels.empty()) {
        auto *first_level =
            new Level(0, this->bit_per_entry, this->input_buffer_capacity, this->output_buffer_capacity);
        this->levels.push_back(first_level);
    }
    this->levels[0]->WriteDataToLevel(data, search_type, kvs_path);
    LSMTree::MaintainLevelCapacityAndCompact(this->levels[0], kvs_path);
}

std::vector<Level *> LSMTree::GetLevels() { return this->levels; }

void LSMTree::AddLevel(Level *level) { this->levels.push_back(level); }

uint64_t LSMTree::Get(uint64_t key, BufferPool *buffer_pool) {
    // Goes through each level in the tree in top-down fashion
    for (Level *level : this->levels) {
        if (level->GetSSTFiles().empty()) {  // Skip empty levels
            continue;
        }

        // Given the size ratio is 2, there is one sst file in this level, so we can search in it.
        // In an LSM tree with size ratio of greater than 2, we would need to make sure we traverse
        // from the most recent file of each level first, but we do not need to handle this in our case.
        for (SST *sst_file : level->GetSSTFiles()) {
            uint64_t value = sst_file->PerformBTreeSearch(key, buffer_pool, true);
            if (value == Utils::DELETED_KEY_VALUE) {
                return Utils::INVALID_VALUE;  // Key does not exist since it has been deleted.
            } else if (value != Utils::INVALID_VALUE) {
                return value;  // Key exists.
            }
        }
    }
    return Utils::INVALID_VALUE;  // Key does not exist.
}

void LSMTree::Scan(uint64_t key1, uint64_t key2, std::vector<DataEntry_t> &scan_result) {
    uint64_t cur_key_to_look_for = key1;
    uint64_t cur_key_to_look_for_counter = 0;
    std::vector<bool> all_levels_scanned(this->levels.size());
    // The 2nd condition captures when are looking for key2 but even after the last level,
    // we could not find it. In this case the key2 does not exist and we stop.
    while (cur_key_to_look_for <= key2 &&
           !(std::all_of(all_levels_scanned.begin(), all_levels_scanned.end(), [](bool v) { return v; }))) {
        int level_Index = 0;
        while (level_Index < this->levels.size()) {
            Level *level = this->levels[level_Index];
            cur_key_to_look_for_counter++;
            if (!level->GetSSTFiles().empty()) {
                // In case of size ratio of 2, there is at most one sst file in each level.
                for (SST *sst_file : level->GetSSTFiles()) {
                    int fd = Utils::OpenFile(sst_file->GetFileName());
                    if (fd == -1) {
                        return;
                    }

                    ScanInputReader *input_reader = sst_file->GetScanInputReader();
                    if (!input_reader->IsLeavesRangeToScanSet()) {
                        uint64_t start_offset_to_scan = SST::ReadBTreeScanLeavesRange(fd, cur_key_to_look_for);
                        input_reader->SetLeavesRangeToScan(start_offset_to_scan, sst_file->GetMaxOffsetToReadLeaves(),
                                                           fd);
                    }

                    if (input_reader->GetInputBufferSize()) {
                        DataEntry_t entry = input_reader->FindKey(cur_key_to_look_for, fd);
                        close(fd);
                        if (entry.second != Utils::INVALID_VALUE) {
                            if (entry.second != Utils::DELETED_KEY_VALUE) {
                                scan_result.push_back(entry);
                            }

                            // The cur_key_to_look_for was found, so proceed to look for the next key.
                            cur_key_to_look_for++;
                            cur_key_to_look_for_counter = 0;
                            level_Index = this->levels.size();
                            break;
                        } else if (cur_key_to_look_for_counter >= this->levels.size()) {
                            // All the levels were checked and the cur_key_to_look_for does not
                            // exist in either of them, so proceed to look for the next key.
                            cur_key_to_look_for++;
                            cur_key_to_look_for_counter = 0;
                        }
                    } else {
                        close(fd);
                    }
                    if (input_reader->IsScannedCompletely()) {
                        all_levels_scanned[level_Index] = true;
                    }
                }
            } else {
                all_levels_scanned[level_Index] = true;
            }
            level_Index++;
        }
    }
}
