#ifndef LEVEL_HPP_
#define LEVEL_HPP_

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <vector>

#include "./avl_tree.hpp"
#include "./btree.hpp"
#include "./buffer_pool.hpp"

namespace fs = std::filesystem;

class Level {
 public:
  std::vector<std::filesystem::path> sst_list;

  // argument "is_last_level" is used for the merge function
  // to know whether to delete tombstones
  void compact(std::filesystem::path db_path, int sst_num, bool is_last_level);

  static void update_levels(std::vector<Level>& levels,
                            std::filesystem::path sst_path,
                            std::filesystem::path db_path,
                            BufferPool& buffer_pool);
  static void load_into_lsm_tree(std::vector<Level>& levels,
                                 std::filesystem::path sst_path);

 private:
  static const int SIZE_RATIO = 2;

  static int extract_number_from_filename(const std::string& file_name);
  static void add_new_level(std::vector<Level>& levels);
};

#endif  // LEVEL_HPP_
