#ifndef BTREELEVEL_H
#define BTREELEVEL_H

#include <vector>

/**
 * Class representing a level in the B-Tree data structure.
 */
class BTreeLevel {
   private:
    /* data */
    uint64_t startingByteOffset;
    uint64_t nextByteOffsetToWrite;
    std::vector<uint64_t> data;

   public:
    explicit BTreeLevel(uint64_t startingByteOffset) {
        this->startingByteOffset = startingByteOffset;
        this->nextByteOffsetToWrite = startingByteOffset;
    }

    [[nodiscard]] uint64_t GetStartingByteOffset() const { return this->startingByteOffset; }

    void AddDataToLevel(uint64_t key) { this->data.push_back(key); }

    std::vector<uint64_t> GetLevelData() { return this->data; }

    void ClearLevel() { this->data.clear(); }

    [[nodiscard]] uint64_t GetNextByteOffsetToWrite() const { return this->nextByteOffsetToWrite; }

    void IncrementNextByteOffsetToWrite(uint64_t bytes) { this->nextByteOffsetToWrite += bytes; }
};

#endif  // BTREELEVEL_H
