#ifndef BUCKET_H
#define BUCKET_H

#include "page.hpp"
#include <cstdint>
#include <forward_list>

class Bucket {
private:
  std::forward_list<Page *> pages;
  // Number of bits used by the bucket so far, out of the total global_depth
  // bits of the hashtable.
  int local_depth;

  // Number of pages mapped to this bucket in total.
  int size;

public:
  explicit Bucket(int depth);

  ~Bucket();

  // Search for the Page object associated with given page id in the bucket
  Page *get_page(const std::string &page_id);

  // Insert a new page object into the bucket chain.
  void insert_page(Page *new_page);

  // Remove the given page object from the bucket chain.
  void remove_page(Page *page_to_remove);

  // Get the number of pages mapped to current bucket.
  int get_size() const;

  // Get the local depth (Number of bits used) of the current bucket.
  int get_local_depth() const;

  // Get all the page objects in the bucket.
  std::forward_list<Page *> get_pages();

  // Increment the local depth of the bucket.
  void increment_local_depth();

  // Decrement the local depth of the bucket.
  void decrement_local_depth();

  // Clear out all the pages stored within the bucket
  void clear();
};

#endif // BUCKET_H
