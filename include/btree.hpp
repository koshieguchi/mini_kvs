#ifndef BTREE_H
#define BTREE_H

#include <fstream>
#include <iostream>
#include <vector>

#include "avl_tree.hpp"
#include "buffer_pool.hpp"
#include "utils.hpp"

class BTreeNode {
 public:
  struct Entry {
    uint32_t key;
    uint32_t value;
  };

  // MAX_KV_PAIRS_PER_PAGE = (PAGE_SIZE - sizeof(num_keys) - sizeof(file_offset)
  // - sizeof(total_number_of_nodes) - num_of_leaf_nodes - sizeof(is_leaf)) / KEY_VALUE_SIZE - 1
  static constexpr int MAX_KEYS =
      (Utils::PAGE_SIZE - sizeof(int) * 4 - sizeof(bool)) / sizeof(Entry) - 1;

  // Current number of key-value pairs in this node
  int num_keys;

  Entry entries[MAX_KEYS + 1];

  // if it's not a leaf node, the value field will store the file
  bool is_leaf;

  // offset to the child node
  int file_offset;

  // metadata (only used by the root node)
  int total_number_of_nodes;
  int num_of_leaf_nodes;

  static void extract_leaf_nodes_from_avl(Node* root,
                                          std::vector<BTreeNode*>& leaf_nodes);
  static BTreeNode* construct_internal_nodes_and_write_to_file(
      std::vector<BTreeNode*>& leaf_nodes, std::ofstream file);
  static uint32_t search_value_by_key(uint32_t key,
                                      std::filesystem::path file_path,
                                      BufferPool* buffer_pool);
  static void scan(uint32_t start_key, uint32_t end_key,
                   std::filesystem::path file_path, BufferPool* buffer_pool,
                   std::vector<std::pair<uint32_t, uint32_t> >* result);
  static void read_leaf_nodes_from_file(std::ifstream& file, int offset,
                                        std::vector<BTreeNode*>& leafNodes);

  // argument "is_last_level" is used for the merge function
  // to know whether to delete tombstones
  static void merge_ssts(std::filesystem::path old_sst_path,
                         std::filesystem::path new_sst_path,
                         std::filesystem::path output_path,
                         bool is_last_level);

 private:
  static BTreeNode* find_node(std::filesystem::path file_path,
                              std::ifstream* file, int offset,
                              BufferPool* buffer_pool);
};

#endif  // BTREE_H
