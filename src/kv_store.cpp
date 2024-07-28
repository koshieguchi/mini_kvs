#include "kv_store.hpp"

#include <fcntl.h>
#include <unistd.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

#include "btree.hpp"
#include "utils.hpp"

KVStore::KVStore(int memtable_size, int initial_size, int max_size)
    : memtable_size(memtable_size), buffer_pool(initial_size, max_size) {}

/*
 * Basic API
 */
void KVStore::open(const std::string &name) {
    db_name = name;
    db_path = fs::current_path() / db_name;

    if (!fs::exists(db_path)) {
        fs::create_directory(db_path);
    } else {
        // reconstruct LSM tree levels based on SST files
        for (const auto &entry : fs::directory_iterator(db_path)) {
            if (fs::is_regular_file(entry)) {
                Level::load_into_lsm_tree(levels, entry.path());
            }
        }
    }
}

void KVStore::put(uint32_t key, uint32_t value) {
    memtable.put(key, value);
    current_memtable_entries++;

    // if memtable is full, write it to SST
    if (current_memtable_entries >= memtable_size) {
        write_memtable_to_sst();
    }
}

uint32_t KVStore::get(uint32_t key) {
    uint32_t value = memtable.get(key);

    if (value == Utils::TOMB_STONE) {
        // Key does not exist since it has been deleted.
        return Utils::INVALID_VALUE;
    } else if (value != Utils::INVALID_VALUE) {
        return value;
    }

    // Search SSTs if you cannot find the key in memtable
    value = find_value_in_ssts(key);
    return value;
}

std::vector<std::pair<uint32_t, uint32_t>> KVStore::scan(uint32_t start_key, uint32_t end_key) {
    std::vector<std::pair<uint32_t, uint32_t>> *result = new std::vector<std::pair<uint32_t, uint32_t>>();

    scan_memtable(memtable.root, start_key, end_key, result);
    scan_ssts(start_key, end_key, result);

    std::sort(
        result->begin(), result->end(),
        [](const std::pair<uint32_t, uint32_t> a, const std::pair<uint32_t, uint32_t> b) { return a.first < b.first; });

    for (auto it = result->begin(); it != result->end();) {
        if (it->second == Utils::TOMB_STONE) {
            it->second = Utils::INVALID_VALUE;
        } else {
            it++;
        }
    }

    return *result;
}

void KVStore::delete_key(uint32_t key) {
    // When deleting a key, we just put a tombstone in the memtable
    memtable.put(key, Utils::TOMB_STONE);
}

void KVStore::close() {
    // when closing, flush memtable to SSTs
    write_memtable_to_sst();
}

/*
 * Helper Functions
 */

void KVStore::scan_memtable(Node *node, uint32_t start_key, uint32_t end_key,
                            std::vector<std::pair<uint32_t, uint32_t>> *result) {
    if (node == nullptr) return;

    if (start_key < node->key) {
        scan_memtable(node->left, start_key, end_key, result);
    }
    if (start_key <= node->key && node->key <= end_key) {
        result->push_back({node->key, node->value});
    }
    if (node->key < end_key) {
        scan_memtable(node->right, start_key, end_key, result);
    }
}

void KVStore::scan_ssts(uint32_t start_key, uint32_t end_key, std::vector<std::pair<uint32_t, uint32_t>> *result) {
    for (auto &level : levels) {
        for (auto &sst_path : level.sst_list) {
            BTreeNode::scan(start_key, end_key, sst_path, &buffer_pool, result);
        }
    }
}

uint32_t KVStore::find_value_in_ssts(uint32_t key) {
    for (auto &level : levels) {
        for (auto &sst_path : level.sst_list) {
            uint32_t value = BTreeNode::search_value_by_key(key, sst_path, &buffer_pool);
            if (value == Utils::TOMB_STONE) {
                return Utils::INVALID_VALUE;  // Key does not exist since it has been
                                              // deleted.
            } else if (value != Utils::INVALID_VALUE) {
                return value;
            }
        }
    }

    return Utils::INVALID_VALUE;
}

void KVStore::write_memtable_to_sst() {
    std::vector<BTreeNode *> leaf_nodes;
    BTreeNode::extract_leaf_nodes_from_avl(memtable.root, leaf_nodes);
    if (leaf_nodes.size() == 0) {
        return;
    }

    int sst_num = 0;
    if (levels.size() > 0 && levels[0].sst_list.size() > 0) {
        sst_num = 1;
    }

    auto file_path = db_path / ("sst_" + std::to_string(sst_num) + ".dat");

    std::ofstream file(file_path, std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + file_path.string());
    }

    // Build internal nodes
    BTreeNode::construct_internal_nodes_and_write_to_file(leaf_nodes, std::move(file));
    file.close();

    sst_count++;
    current_memtable_entries = 0;
    memtable.clear();

    Level::update_levels(levels, file_path, db_path, buffer_pool);
}
