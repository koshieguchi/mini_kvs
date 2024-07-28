#include "lsm_tree.h"

#include <unistd.h>

#include <bitset>
#include <functional>
#include <queue>

LSMTree::LSMTree(int bloomFilterBitsPerEntry, int inputBufferCapacity, int outputBufferCapacity) {
    this->levels = {};
    this->bitPerEntry = bloomFilterBitsPerEntry;
    this->inputBufferCapacity = inputBufferCapacity;
    this->outputBufferCapacity = outputBufferCapacity;
}

LSMTree::~LSMTree() {
    for (auto level : this->levels) {
        delete level;
    }
}

void LSMTree::MaintainLevelCapacityAndCompact(Level *currLevel, std::string &dbPath) {
    int level = currLevel->GetLevelNumber();
    if (this->levels[level]->GetSSTFiles().size() <= 1) {
        return;
    }

    if (level + 1 >= this->levels.size()) {
        auto *newLevel = new Level(level + 1, this->bitPerEntry, this->inputBufferCapacity, this->outputBufferCapacity);
        this->levels.push_back(newLevel);
    }

    Level *nextLevel = this->levels[level + 1];
    currLevel->SortMergeAndWriteToNextLevel(nextLevel, dbPath);
    LSMTree::MaintainLevelCapacityAndCompact(nextLevel, dbPath);
}

void LSMTree::WriteMemtableData(std::vector<DataEntry_t> &data, SearchType searchType, std::string &dbPath) {
    // Always write the new sst files to the first level
    if (this->levels.empty()) {
        auto *firstLevel = new Level(0, this->bitPerEntry, this->inputBufferCapacity, this->outputBufferCapacity);
        this->levels.push_back(firstLevel);
    }
    this->levels[0]->WriteDataToLevel(data, searchType, dbPath);
    LSMTree::MaintainLevelCapacityAndCompact(this->levels[0], dbPath);
}

std::vector<Level *> LSMTree::GetLevels() { return this->levels; }

void LSMTree::AddLevel(Level *level) { this->levels.push_back(level); }

uint64_t LSMTree::Get(uint64_t key, BufferPool *bufferPool) {
    // Goes through each level in the tree in top-down fashion
    for (Level *level : this->levels) {
        if (level->GetSSTFiles().empty()) {  // Skip empty levels
            continue;
        }

        // Given the size ratio is 2, there is one sst file in this level, so we can search in it.
        // In an LSM tree with size ratio of greater than 2, we would need to make sure we traverse
        // from the most recent file of each level first, but we do not need to handle this in our case.
        for (SST *sstFile : level->GetSSTFiles()) {
            uint64_t value = sstFile->PerformBTreeSearch(key, bufferPool, true);
            if (value == Utils::DELETED_KEY_VALUE) {
                return Utils::INVALID_VALUE;  // Key does not exist since it has been deleted.
            } else if (value != Utils::INVALID_VALUE) {
                return value;  // Key exists.
            }
        }
    }
    return Utils::INVALID_VALUE;  // Key does not exist.
}

void LSMTree::Scan(uint64_t key1, uint64_t key2, std::vector<DataEntry_t> &scanResult) {
    uint64_t curKeyToLookFor = key1;
    uint64_t curKeyToLookForCounter = 0;
    std::vector<bool> allLevelsScanned(this->levels.size());
    // The 2nd condition captures when are looking for key2 but even after the last level,
    // we could not find it. In this case the key2 does not exist and we stop.
    while (curKeyToLookFor <= key2 &&
           !(std::all_of(allLevelsScanned.begin(), allLevelsScanned.end(), [](bool v) { return v; }))) {
        int levelIndex = 0;
        while (levelIndex < this->levels.size()) {
            Level *level = this->levels[levelIndex];
            curKeyToLookForCounter++;
            if (!level->GetSSTFiles().empty()) {
                // In case of size ratio of 2, there is at most one sst file in each level.
                for (SST *sstFile : level->GetSSTFiles()) {
                    int fd = Utils::OpenFile(sstFile->GetFileName());
                    if (fd == -1) {
                        return;
                    }

                    ScanInputReader *inputReader = sstFile->GetScanInputReader();
                    if (!inputReader->IsLeavesRangeToScanSet()) {
                        uint64_t startOffsetToScan = SST::ReadBTreeScanLeavesRange(fd, curKeyToLookFor);
                        inputReader->SetLeavesRangeToScan(startOffsetToScan, sstFile->GetMaxOffsetToReadLeaves(), fd);
                    }

                    if (inputReader->GetInputBufferSize()) {
                        DataEntry_t entry = inputReader->FindKey(curKeyToLookFor, fd);
                        close(fd);
                        if (entry.second != Utils::INVALID_VALUE) {
                            if (entry.second != Utils::DELETED_KEY_VALUE) {
                                scanResult.push_back(entry);
                            }

                            // The curKeyToLookFor was found, so proceed to look for the next key.
                            curKeyToLookFor++;
                            curKeyToLookForCounter = 0;
                            levelIndex = this->levels.size();
                            break;
                        } else if (curKeyToLookForCounter >= this->levels.size()) {
                            // All the levels were checked and the curKeyToLookFor does not
                            // exist in either of them, so proceed to look for the next key.
                            curKeyToLookFor++;
                            curKeyToLookForCounter = 0;
                        }
                    } else {
                        close(fd);
                    }
                    if (inputReader->IsScannedCompletely()) {
                        allLevelsScanned[levelIndex] = true;
                    }
                }
            } else {
                allLevelsScanned[levelIndex] = true;
            }
            levelIndex++;
        }
    }
}
