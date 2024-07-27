#include "btree.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <vector>

#include "bloom_filter.hpp"
#include "kv_store.hpp"
#include "utils.hpp"

// Function to convert AVL tree to sorted list of leaf nodes
// basically we keep adding key-value pairs to a leaf node
// once the leaf node is full, we move on to the next one
void BTreeNode::extract_leaf_nodes_from_avl(
    Node* root, std::vector<BTreeNode*>& leaf_nodes) {
  if (root == nullptr) {
    return;
  }

  extract_leaf_nodes_from_avl(root->left, leaf_nodes);

  // if leaf nodes vector is empty, or the current B tree node we are inserting
  // to is full
  //  we need to make a new B tree node
  if (leaf_nodes.empty() ||
      leaf_nodes.back()->num_keys == BTreeNode::MAX_KEYS) {
    leaf_nodes.push_back(new BTreeNode());
  }
  BTreeNode* leaf_node = leaf_nodes.back();

  leaf_node->entries[leaf_node->num_keys].key = root->key;
  leaf_node->entries[leaf_node->num_keys].value = root->value;
  leaf_node->num_keys++;
  leaf_node->is_leaf = true;

  extract_leaf_nodes_from_avl(root->right, leaf_nodes);
}

int get_total_number_of_nodes(int num_of_leaf_nodes) {
  if (num_of_leaf_nodes == 1) {
    // this ensures that there is always a root node
    return 2;
  }
  int current_level_node_count = num_of_leaf_nodes;
  int total_number_of_nodes = num_of_leaf_nodes;
  while (current_level_node_count > 1) {
    // the number of internal nodes of the next level is ceil(number of children
    // / (MAX + 1)) note that we add 1 to MAX because of how delimiter works
    current_level_node_count =
        std::ceil(current_level_node_count / (BTreeNode::MAX_KEYS + 1.0));
    total_number_of_nodes += current_level_node_count;
  }
  return total_number_of_nodes;
}

// Function to build internal nodes from a list of leaf nodes, and also write
// them to file simultaneously
BTreeNode* BTreeNode::construct_internal_nodes_and_write_to_file(
    std::vector<BTreeNode*>& leaf_nodes, std::ofstream file) {
  // bloom filter
  BloomFilter filter(
      5, leaf_nodes.size());  // Number of bits in the bloom filter = 5
  for (BTreeNode* node : leaf_nodes) {
    for (int i = 0; i < node->num_keys; i++) {
      filter.insert(node->entries[i].key);
    }
  }
  file.seekp(0);
  file.write((char*)&filter, sizeof(BloomFilter));

  // get total number of nodes
  // this is because we are processing nodes starting from the leaves, and then
  // all the way up to the root since the root must be at offset 0, the leaf
  // nodes must be at the maximum offset therefore, we need to compute the total
  // number of nodes to get the maximum offset
  int num_of_leaf_nodes = leaf_nodes.size();
  int total_number_of_nodes = get_total_number_of_nodes(num_of_leaf_nodes);
  int node_offset = total_number_of_nodes;

  for (BTreeNode* node : leaf_nodes) {
    node->file_offset = node_offset;
    node_offset--;

    // make sure nodes are page aligned
    file.seekp(Utils::PAGE_SIZE * node->file_offset);
    file.write((char*)node, sizeof(BTreeNode));
  }
  // we process all of the leaf nodes in order to create one level
  // we add all of the internal nodes of that level into result
  // then we check if the size of result == 1, if so we have reached the root
  // and we're done, otherwise we process result recursively
  std::vector<BTreeNode*> queue = leaf_nodes;

  while (true) {
    std::vector<BTreeNode*> result;
    while (!queue.empty()) {
      // Create a new internal node
      BTreeNode* internal_node = new BTreeNode();
      // because of how delimiters work, a node can have at most MAX_KEYS + 1
      // children
      int n = std::min(BTreeNode::MAX_KEYS + 1, (int)queue.size());
      // note that we are still adding the last child node because we need the
      // offset
      for (int i = 0; i < n; i++) {
        BTreeNode* node = queue[i];
        // get the largest/right-most key of the child node
        internal_node->entries[i].key = node->entries[node->num_keys - 1].key;
        internal_node->entries[i].value = node->file_offset;
      }
      internal_node->num_keys = n;
      // pop from queue
      for (int i = 0; i < n; i++) {
        queue.erase(queue.begin());
      }

      internal_node->file_offset = node_offset;
      node_offset--;

      // make sure nodes are page aligned
      // note that the root should have an offset of exactly one page (because
      // the first page stores the bloom filter)
      file.seekp(Utils::PAGE_SIZE * internal_node->file_offset);
      file.write((char*)internal_node, sizeof(BTreeNode));

      result.push_back(internal_node);
    }
    if (result.size() == 1) {
      // write metadata to root node
      BTreeNode* root = result.front();
      root->total_number_of_nodes = total_number_of_nodes;
      root->num_of_leaf_nodes = num_of_leaf_nodes;
      file.seekp(Utils::PAGE_SIZE);
      file.write((char*)root, sizeof(BTreeNode));
      return result.front();
    }
    queue = result;
  }
}

uint32_t BTreeNode::search_value_by_key(uint32_t key,
                                        std::filesystem::path file_path,
                                        BufferPool* buffer_pool) {
  std::ifstream file;

  // bloom filter
  BloomFilter* filter;
  std::string filter_id = Page::generate_page_id(file_path, 0);
  const char* page_data = buffer_pool->get(filter_id);
  if (page_data == nullptr) {
    // not in buffer pool, we have to access the file
    if (!file.is_open()) {
      file.open(file_path, std::ios::binary | std::ios::in);
    }
    page_data = new char[Utils::PAGE_SIZE];
    file.seekg(0);
    file.read((char*)page_data, sizeof(BloomFilter));
    // add to buffer pool
    buffer_pool->insert(filter_id, page_data);
  }
  filter = (BloomFilter*)page_data;
  if (!filter->get(key)) {
    return Utils::INVALID_VALUE;
  }

  BTreeNode* node;
  int offset = 1;  // root node offset is 1

  // Continue searching until the correct node is found or search concludes
  while (true) {
    node = find_node(file_path, &file, offset, buffer_pool);

    // If the node has no keys, the search is unsuccessful
    if (node->num_keys == 0) {
      return Utils::INVALID_VALUE;
    }

    // If the current node is a leaf, perform binary search
    if (node->is_leaf) {
      int low = 0, high = node->num_keys - 1;
      while (low <= high) {
        int mid = low + (high - low) / 2;
        // Check if the mid value matches the key
        if (node->entries[mid].key == key) {
          return node->entries[mid].value;
        } else if (node->entries[mid].key < key) {
          low = mid + 1;
        } else {
          high = mid - 1;
        }
      }
      // Key not found in the leaf node
      return Utils::INVALID_VALUE;
    } else {
      // For internal nodes, find the correct child to follow
      bool is_key_included = false;

      int low = 0, high = node->num_keys - 1;
      while (low <= high) {
        int mid = low + (high - low) / 2;
        // Choose the child node whose range includes the key
        if (node->entries[mid].key >= key) {
          if (mid == 0 || node->entries[mid - 1].key < key) {
            offset = node->entries[mid].value;
            is_key_included = true;
            break;
          }
          high = mid - 1;
        } else {
          low = mid + 1;
        }
      }
      // If no appropriate child is found, the key is not present
      if (!is_key_included) {
        return Utils::INVALID_VALUE;
      }
    }
  }
}

void BTreeNode::read_leaf_nodes_from_file(std::ifstream& file, int offset,
                                          std::vector<BTreeNode*>& leaf_nodes) {
  BTreeNode* node = new BTreeNode();
  file.seekg(Utils::PAGE_SIZE * offset);
  file.read(reinterpret_cast<char*>(node), sizeof(BTreeNode));

  if (node->is_leaf) {
    leaf_nodes.push_back(node);
  } else {
    for (int i = 0; i < node->num_keys; ++i) {
      int child_offset = node->entries[i].value;
      read_leaf_nodes_from_file(file, child_offset, leaf_nodes);
    }
    delete node;
  }
}

void BTreeNode::merge_ssts(std::filesystem::path old_sst_path,
                           std::filesystem::path new_sst_path,
                           std::filesystem::path output_path,
                           bool is_last_level) {
  std::ifstream old_sst(old_sst_path, std::ios::binary);
  std::ifstream new_sst(new_sst_path, std::ios::binary);
  std::ofstream output(output_path, std::ios::binary);
  // write leaf nodes to a temp file
  std::filesystem::path temp_path = output_path.replace_extension(".tmp");
  std::ofstream temp_output(temp_path, std::ios::binary);

  // get the root nodes
  BTreeNode* old_root = new BTreeNode();
  old_sst.seekg(Utils::PAGE_SIZE);
  old_sst.read((char*)old_root, sizeof(BTreeNode));
  BTreeNode* new_root = new BTreeNode();
  new_sst.seekg(Utils::PAGE_SIZE);
  new_sst.read((char*)new_root, sizeof(BTreeNode));

  // we need to know exactly the total number of nodes in order to determine the
  // correct offset to write the leaf nodes but we can't know that in advance
  // unless we iterate through all of the nodes because there could be updates
  // and deletes therefore, we write to a temp file (from the beginning) in
  // order to get the number of leaf nodes
  int offset = 0;
  int old_offset = old_root->total_number_of_nodes;
  int new_offset = new_root->total_number_of_nodes;
  int old_min_offset =
      old_root->total_number_of_nodes - old_root->num_of_leaf_nodes + 1;
  int new_min_offset =
      new_root->total_number_of_nodes - new_root->num_of_leaf_nodes + 1;
  BTreeNode* old_node = new BTreeNode();
  old_sst.seekg(Utils::PAGE_SIZE * old_offset);
  old_sst.read((char*)old_node, sizeof(BTreeNode));
  int old_index = 0;
  BTreeNode* new_node = new BTreeNode();
  new_sst.seekg(Utils::PAGE_SIZE * new_offset);
  new_sst.read((char*)new_node, sizeof(BTreeNode));
  int new_index = 0;
  BTreeNode* output_node = new BTreeNode();
  int output_index = 0;
  BTreeNode::Entry invalid_entry = {Utils::INVALID_VALUE, Utils::INVALID_VALUE};
  while (true) {
    // an invalid entry is used if we ran out of leaf nodes for one of them
    BTreeNode::Entry old_entry = old_index >= old_node->num_keys
                                     ? invalid_entry
                                     : old_node->entries[old_index];
    BTreeNode::Entry new_entry = new_index >= new_node->num_keys
                                     ? invalid_entry
                                     : new_node->entries[new_index];
    if (old_entry.value == Utils::TOMB_STONE && is_last_level && (new_entry.key == old_entry.key)) {
      // When tombstones reach the largest level of the LSM-tree, they should be removed,
      // as at this point there are no longer any older versions of the entry in existence
      // for them to mask.
      old_index++;
      new_index++;
    } else if (old_entry.key == Utils::INVALID_VALUE) {
      output_node->entries[output_index].key = new_entry.key;
      output_node->entries[output_index].value = new_entry.value;
      new_index++;
    } else if (new_entry.key == Utils::INVALID_VALUE) {
      output_node->entries[output_index].key = old_entry.key;
      output_node->entries[output_index].value = old_entry.value;
      old_index++;
    } else if (new_entry.key < old_entry.key) {
      output_node->entries[output_index].key = new_entry.key;
      output_node->entries[output_index].value = new_entry.value;
      new_index++;
    } else if (new_entry.key > old_entry.key) {
      output_node->entries[output_index].key = old_entry.key;
      output_node->entries[output_index].value = old_entry.value;
      old_index++;
    } else {
      // they have the same key, always prefer the new one
      output_node->entries[output_index].key = new_entry.key;
      output_node->entries[output_index].value = new_entry.value;
      new_index++;
      old_index++;
    }
    // check if we have exhausted the entries for a node
    // we also check if we have gone through all leaf nodes already
    if (old_index >= old_node->num_keys && old_offset > old_min_offset) {
      old_offset--;
      old_sst.seekg(Utils::PAGE_SIZE * old_offset);
      old_sst.read((char*)old_node, sizeof(BTreeNode));
      old_index = 0;
    }
    if (new_index >= new_node->num_keys && new_offset > new_min_offset) {
      new_offset--;
      new_sst.seekg(Utils::PAGE_SIZE * new_offset);
      new_sst.read((char*)new_node, sizeof(BTreeNode));
      new_index = 0;
    }
    output_index++;
    // check if we have filled up a node, or we are done
    //  if we are done, both indexes would be 0
    bool done =
        old_index >= old_node->num_keys && new_index >= new_node->num_keys;
    if (output_index >= BTreeNode::MAX_KEYS || done) {
      output_node->num_keys = output_index;
      output_node->is_leaf = true;
      // output_node->file_offset = offset;
      temp_output.seekp(Utils::PAGE_SIZE * offset);
      temp_output.write((char*)output_node, sizeof(BTreeNode));
      offset++;
      if (done) {
        break;
      }
      output_node = new BTreeNode();
      output_index = 0;
    }
  }
  temp_output.close();

  // offset now holds the number of leaf nodes/pages
  int num_of_leaf_nodes = offset;
  int total_number_of_nodes = get_total_number_of_nodes(num_of_leaf_nodes);
  // bloom filter
  BloomFilter filter(5, num_of_leaf_nodes);
  // now read the nodes back from the temp file
  std::ifstream temp_input(temp_path, std::ios::binary);
  offset = total_number_of_nodes;  // we write the leaf nodes backwards
  int input_offset = 0;
  std::vector<BTreeNode::Entry>
      queue;  // we only need to know the largest key and its node's offset (to
              // save memory)
  // write leaf nodes from temp file to merge output file
  while (input_offset < num_of_leaf_nodes) {
    temp_input.seekg(Utils::PAGE_SIZE * input_offset);
    temp_input.read((char*)output_node, sizeof(BTreeNode));
    output_node->file_offset = offset;
    for (int i = 0; i < output_node->num_keys; i++) {
      filter.insert(output_node->entries[i].key);
    }
    output.seekp(Utils::PAGE_SIZE * offset);
    output.write((char*)output_node, sizeof(BTreeNode));
    BTreeNode::Entry entry = {
        output_node->entries[output_node->num_keys - 1].key, (uint32_t)offset};
    queue.push_back(entry);
    input_offset++;
    offset--;
  }

  // write bloom filter
  output.seekp(0);
  output.write((char*)&filter, sizeof(BloomFilter));

  // delete temp file
  std::filesystem::remove(temp_path);

  // build internal nodes based on leaf nodes
  // we had an in-memory vector (the queue) because we can't read and write to
  // the same file
  // reset offset
  offset = total_number_of_nodes - num_of_leaf_nodes;

  while (true) {
    std::vector<BTreeNode::Entry> result;
    BTreeNode* internal_node;
    while (!queue.empty()) {
      internal_node = new BTreeNode();
      BTreeNode::Entry entry;
      int n = std::min(BTreeNode::MAX_KEYS + 1, (int)queue.size());
      for (int i = 0; i < n; i++) {
        entry = queue[i];
        internal_node->entries[i].key = entry.key;
        internal_node->entries[i].value = entry.value;
      }
      internal_node->num_keys = n;
      // pop from queue
      for (int i = 0; i < n; i++) {
        queue.erase(queue.begin());
      }
      internal_node->file_offset = offset;
      offset--;
      output.seekp(Utils::PAGE_SIZE * internal_node->file_offset);
      output.write((char*)internal_node, sizeof(BTreeNode));
      entry = {internal_node->entries[internal_node->num_keys - 1].key,
               (uint32_t)internal_node->file_offset};
      result.push_back(entry);
    }
    if (result.size() == 1) {
      // we have written the root node, write metadata
      internal_node->total_number_of_nodes = total_number_of_nodes;
      internal_node->num_of_leaf_nodes = num_of_leaf_nodes;
      output.seekp(Utils::PAGE_SIZE);
      output.write((char*)internal_node, sizeof(BTreeNode));
      // and then we're done!
      return;
    }
    queue = result;
  }
}

// find B tree node in either the buffer pool, or by getting its page from the
// SST directly
BTreeNode* BTreeNode::find_node(std::filesystem::path file_path,
                                std::ifstream* file, int offset,
                                BufferPool* buffer_pool) {
  std::string page_id = Page::generate_page_id(file_path, offset);
  const char* page_data = buffer_pool->get(page_id);
  if (page_data == nullptr) {
    // not in buffer pool, we have to access the file
    if (!file->is_open()) {
      file->open(file_path, std::ios::binary | std::ios::in);
    }
    file->seekg(offset * Utils::PAGE_SIZE);
    page_data = (const char*)(new BTreeNode());
    file->read((char*)page_data, sizeof(BTreeNode));
    // add to buffer pool
    buffer_pool->insert(page_id, page_data);
  }
  return (BTreeNode*)page_data;
}

void BTreeNode::scan(uint32_t start_key, uint32_t end_key,
                     std::filesystem::path file_path, BufferPool* buffer_pool,
                     std::vector<std::pair<uint32_t, uint32_t>>* result) {
  std::ifstream file;
  BTreeNode* node;
  int offset = 1;  // root node offset is 1

  // Continue searching until the correct node is found or search concludes
  while (true) {
    node = find_node(file_path, &file, offset, buffer_pool);

    // we found the node that contains the smallest key that fits in the range
    if (node->is_leaf) {
      break;
    }

    // Find the correct child to follow
    bool is_key_included = false;

    int low = 0, high = node->num_keys - 1;
    while (low <= high) {
      int mid = low + (high - low) / 2;
      // Choose the child node whose range includes the key
      if (node->entries[mid].key >= start_key) {
        if (mid == 0 || node->entries[mid - 1].key < start_key) {
          offset = node->entries[mid].value;
          is_key_included = true;
          break;
        }
        high = mid - 1;
      } else {
        low = mid + 1;
      }
    }
    // If no appropriate child is found, the key is not present
    if (!is_key_included) {
      return;
    }
  }

  // we keep going to the next page (because leaf nodes are stored contiguously)
  while (node->is_leaf) {
    for (int i = 0; i < node->num_keys; i++) {
      BTreeNode::Entry entry = node->entries[i];
      if (entry.key < start_key) {
        continue;
      } else if (entry.key > end_key) {
        return;
      }
      result->push_back({entry.key, entry.value});
    }
    offset--;  // we subtract because that's how the offsets were calculated
               // when we are writing the B Tree to the SST
    node = find_node(file_path, &file, offset, buffer_pool);
  }
}
