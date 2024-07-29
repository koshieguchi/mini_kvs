#ifndef BTREE_LEVEL_H
#define BTREE_LEVEL_H

#include <vector>

/**
 * Class representing a level in the B-Tree data structure.
 */
class BTreeLevel {
   private:
    uint64_t starting_byte_offset;
    uint64_t next_byte_offset_to_write;
    std::vector<uint64_t> data;

   public:
    explicit BTreeLevel(uint64_t starting_byte_offset) {
        this->starting_byte_offset = starting_byte_offset;
        this->next_byte_offset_to_write = starting_byte_offset;
    }

    [[nodiscard]] uint64_t GetStartingByteOffset() const { return this->starting_byte_offset; }

    void AddDataToLevel(uint64_t key) { this->data.push_back(key); }

    std::vector<uint64_t> GetLevelData() { return this->data; }

    void ClearLevel() { this->data.clear(); }

    [[nodiscard]] uint64_t GetNextByteOffsetToWrite() const { return this->next_byte_offset_to_write; }

    void IncrementNextByteOffsetToWrite(uint64_t bytes) { this->next_byte_offset_to_write += bytes; }
};

#endif  // BTREE_LEVEL_H
