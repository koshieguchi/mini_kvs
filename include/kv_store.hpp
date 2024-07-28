#ifndef KV_STORE_HPP_
#define KV_STORE_HPP_

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

#include "./avl_tree.hpp"
#include "./buffer_pool.hpp"
#include "./level.hpp"

namespace fs = std::filesystem;

class KVStore {
   private:
    AVLTree memtable;
    int memtable_size;  // max number of entries in memtable
    int current_memtable_entries = 0;
    std::string db_name;
    fs::path db_path;
    int sst_count = 0;
    std::vector<Level> levels;

    BufferPool buffer_pool;

   public:
    KVStore(int memtable_size, int initial_size, int max_size);

    // Basic API Functions
    void open(const std::string &name);
    void put(uint32_t key, uint32_t value);
    uint32_t get(uint32_t key);
    std::vector<std::pair<uint32_t, uint32_t>> scan(uint32_t start_key, uint32_t end_key);
    void delete_key(uint32_t key);  // name "delete" will conflict with C++ keyword
    void close();

    // Helper Functions
    void scan_memtable(Node *node, uint32_t start_key, uint32_t end_key,
                       std::vector<std::pair<uint32_t, uint32_t>> *result);
    void scan_ssts(uint32_t start_key, uint32_t end_key, std::vector<std::pair<uint32_t, uint32_t>> *result);
    uint32_t find_value_in_ssts(uint32_t key);
    void write_memtable_to_sst();
};

#endif  // KV_STORE_HPP_
