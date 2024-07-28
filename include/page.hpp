#ifndef PAGE_HPP_
#define PAGE_HPP_

#include <cstdint>
#include <filesystem>
#include <map>
#include <string>
#include <utility>
#include <vector>

class Page {
 private:
  std::string page_id;
  const char* data;

 public:
  explicit Page(const std::string& page_id, const char* data) {
    this->page_id = page_id;
    this->data = data;
  }

  // Get the page id of the current page
  std::string get_page_id() const { return this->page_id; }

  // Get all the key-value data within the page. the keys are on even indices
  // while values are on odd indices
  const char* get_data() const { return this->data; }

  // generate page id
  static std::string generate_page_id(std::filesystem::path sst_path,
                                      int offset) {
    return sst_path.string() + "-" + std::to_string(offset);
  }
};

#endif  // PAGE_HPP_
