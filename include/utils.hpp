#ifndef UTILS_HPP_
#define UTILS_HPP_

#include <bitset>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>

namespace Utils {
;
const uint32_t INVALID_VALUE = std::numeric_limits<uint32_t>::max();   // = -1
const uint32_t TOMB_STONE = std::numeric_limits<uint32_t>::max() - 1;  // tomb stone for deletion in LSMTree
const int ENTRY_SIZE = 2 * sizeof(uint32_t);
const int PAGE_SIZE = 4096;

std::string get_binary_from_int(uint32_t integer, int num_bits);

// This function is mainly used for testing and experiments.
void clear_databases(std::string db_dir_path, std::string prefix);

}  // namespace Utils

#endif  // UTILS_HPP_
