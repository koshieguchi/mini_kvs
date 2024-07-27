#include "level.hpp"

#include <fcntl.h>
#include <unistd.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <vector>

#include "btree.hpp"
#include "kv_store.hpp"
#include "utils.hpp"

int Level::extract_number_from_filename(const std::string& file_name) {
  // Use a regular expression to match the numerical part of the file name
  std::regex regex("sst_(\\d+)\\.dat");
  std::smatch match;

  if (std::regex_search(file_name, match, regex)) {
    std::string num = match[1];
    return std::stoi(num);
  }
  return -1;
}

void Level::compact(std::filesystem::path db_path, int sst_num,
                    bool is_last_level) {
  std::filesystem::path new_sst_path =
      db_path / ("sst_" + std::to_string(sst_num) + ".dat");

  // TODO rewrite the merging code to use buffers and handle updates
  BTreeNode::merge_ssts(sst_list[0], sst_list[1], new_sst_path, is_last_level);

  // Remove old SSTables
  for (const auto& old_sst_path : sst_list) {
    if (!std::filesystem::remove(old_sst_path)) {
      std::cerr << "Failed to remove old SSTable: " << old_sst_path
                << std::endl;
    }
  }

  // Update sstables vector
  sst_list.clear();
  sst_list.push_back(new_sst_path);
}

void Level::update_levels(std::vector<Level>& levels,
                          std::filesystem::path sst_path,
                          std::filesystem::path db_path,
                          BufferPool& buffer_pool) {
  if (levels.size() == 0) {
    add_new_level(levels);
  }
  // add to current level
  levels[0].sst_list.push_back(sst_path);
  // compaction, we use a while loop because compaction can happen recursively
  int current_level = 0;
  while (current_level < (int)levels.size()) {
    // Level& level = levels[current_level];
    if ((int)levels[current_level].sst_list.size() >= SIZE_RATIO) {
      // need to compact
      bool is_last_level = (current_level == (int)levels.size() - 1);

      //  if we are already at the last level, then we have to add a new level
      if (current_level >= (int)levels.size() - 1) {
        add_new_level(levels);
      }

      // remove SSTs from the buffer pool because their pages are no longer
      // valid
      for (auto& sst : levels[current_level].sst_list) {
        // get the number of pages the sst has based on its file size
        int num_pages = std::ceil(std::filesystem::file_size(sst) /
                                  (double)Utils::PAGE_SIZE);
        for (int i = 0; i < num_pages; i++) {
          buffer_pool.remove(Page::generate_page_id(sst, i));
        }
      }

      // generate the new SST number
      int new_sst_num = (current_level + 1) * SIZE_RATIO;
      new_sst_num += levels[current_level + 1].sst_list.size();

      // check if we are at the last level
      // this is used for the merge function to know whether to delete tombstones
      levels[current_level].compact(db_path, new_sst_num, is_last_level);
      // note that now the level itself contains the compacted SST
      //  we now manually move it to the next level
      levels[current_level + 1].sst_list.push_back(
          levels[current_level].sst_list[0]);
      // clear the current level
      levels[current_level].sst_list.clear();
      // move on to the next level
      current_level++;
    } else {
      // if there is no need to compact, then there's no need to keep going
      // through the levels to check for compaction we stop
      return;
    }
  }
}

void Level::load_into_lsm_tree(std::vector<Level>& levels,
                               std::filesystem::path sst_path) {
  int sst_num = extract_number_from_filename(sst_path);
  // because the SST numbers are static, we can use them to infer the
  // structure of the LSM tree
  int level = sst_num / SIZE_RATIO;
  while ((int)levels.size() <= level) {
    add_new_level(levels);
  }
  levels[level].sst_list.push_back(sst_path);
}

void Level::add_new_level(std::vector<Level>& levels) {
  Level new_level;
  levels.push_back(new_level);
}
