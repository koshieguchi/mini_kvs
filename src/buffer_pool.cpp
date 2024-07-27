#include "buffer_pool.hpp"

#include <cmath>
#include <utility>

#include "lru.hpp"
#include "utils.hpp"

BufferPool::BufferPool(int min_size, int max_size) {
  this->hashtable = new ExtendibleHashtable(min_size, max_size);
  this->eviction_policy = new LRU();
}

BufferPool::~BufferPool() {
  /*
    We don't need to delete the hashtable since the hashtable destructor will
    de-allocate all the pages stored within the hashtable.
   */

  delete this->eviction_policy;
}

const char *BufferPool::get(const std::string &page_id) {
  // std::vector<uint32_t> page_data;
  Page *accessed_page = this->hashtable->get_page(page_id);
  if (accessed_page != nullptr) {
    this->eviction_policy->update(accessed_page->get_page_id());
    return accessed_page->get_data();
  }
  return {};
}

void BufferPool::resize(int new_max_size) {
  int num_to_evict =
      std::ceil(this->hashtable->get_size() -
                (ExtendibleHashtable::EXPANSION_THRESHOLD * new_max_size));
  if (num_to_evict > 0) {
    for (int i = 0; i < num_to_evict; i++) {
      this->evict();
    }

    // Shrink the hashtable
    this->hashtable->shrink_directory();
  }
  this->hashtable->set_max_size(new_max_size);
}

void BufferPool::insert(const std::string &page_id, const char *data) {
  // Expand the directory if the total number of pages mapped to this hash table
  // is greater than a certain directory size threshold.
  if (this->hashtable->get_size() >
      this->hashtable->get_num_directory() *
          ExtendibleHashtable::EXPANSION_THRESHOLD) {
    bool need_to_evict = !this->hashtable->expand_directory();
    if (need_to_evict) {
      // Evict when directory can't be expanded anymore
      this->evict();
    }
  }

  Page *new_page = new Page(page_id, data);
  this->hashtable->insert_page(new_page);
  this->eviction_policy->insert(new_page->get_page_id());
}

void BufferPool::evict() {
  Page *page_to_evict =
      this->hashtable->get_page(this->eviction_policy->evict());
  this->hashtable->remove_page(page_to_evict);
}

std::vector<Page *> BufferPool::get_all_pages() {
  return hashtable->get_all_pages();
}

void BufferPool::remove(const std::string &page_id) {
  Page *page = this->hashtable->get_page(page_id);
  if (page != nullptr) {
    this->hashtable->remove_page(page);
    this->eviction_policy->remove(page_id);
  }
}