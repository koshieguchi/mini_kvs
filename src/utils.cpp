
#include "utils.hpp"
#include <fcntl.h>
#include <filesystem>
namespace fs = std::filesystem;

namespace Utils {
  std::string get_binary_from_int(uint32_t integer, int num_bits) {
    std::string result = std::bitset<32>(integer).to_string();
    return result.substr(result.length() - num_bits);
  }

  void clear_databases(std::string db_dir_path,std::string prefix) {
    fs::path test_db_path = fs::current_path() / fs::path(db_dir_path);

    for (const auto& entry : fs::directory_iterator(test_db_path)) {
      if (entry.is_directory() && entry.path().filename().string().find(prefix) == 0) {
        fs::remove_all(entry.path());
      }
    }
  }

} // namespace Utils
