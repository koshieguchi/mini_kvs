#ifndef BUFFER_POOL_H
#define BUFFER_POOL_H

#include <cstdint>
#include <string>

#include "extendible_hashtable.hpp"
#include "lru.hpp"

class BufferPool {
 private:
  ExtendibleHashtable *hashtable;
  LRU *eviction_policy;

  void evict();

 public:
  BufferPool(int min_size, int max_size);

  ~BufferPool();

  // Retrieve the page data associated with a given page ID.
  const char *get(const std::string &page_id);

  // Resize the buffer pool to a new maximum size.
  // Evicts pages if the new size is smaller than the current number of pages.
  void resize(int new_max_size);

  // Insert a new page into the buffer pool.
  void insert(const std::string &page_id, const char *data);

  //Remove a page from the buffer pool
  void remove(const std::string &page_id);

  std::vector<Page *> get_all_pages();
};

#endif  // BUFFER_POOL_H
