#include "btree.hpp"

#include <cassert>
#include <fstream>
#include <iostream>

#include "kv_store.hpp"
#include "utils.hpp"
#include "utils/test_utils.hpp"

void test_get_and_put() {
  KVStore kvstore(21, 2, 4);
  kvstore.open("tests/test_db_1");

  kvstore.put(1, 4);
  kvstore.put(2, 1);
  kvstore.put(7, 1);
  kvstore.put(16, 7);
  kvstore.put(21, 6);
  kvstore.put(22, 3);
  kvstore.put(24, 6);
  kvstore.put(29, 4);
  kvstore.put(31, 3);
  kvstore.put(32, 2);
  kvstore.put(33, 6);
  kvstore.put(35, 7);
  kvstore.put(40, 5);
  kvstore.put(61, 1);
  kvstore.put(73, 5);
  kvstore.put(74, 3);
  kvstore.put(82, 4);
  kvstore.put(90, 2);
  kvstore.put(94, 7);
  kvstore.put(95, 5);
  kvstore.put(97, 2);

  assert(kvstore.get(1) == 4);
  assert(kvstore.get(40) == 5);
  assert(kvstore.get(97) == 2);
  assert(kvstore.get(100) == Utils::INVALID_VALUE);

  std::cout << "test_get_and_put passed!" << std::endl;
}

void test_scan() {
  // Scan only on memtable
  KVStore kvstore(10, 2, 4);
  kvstore.open("tests/test_db_5");

  for (int i = 1; i <= 9; i++) {
    kvstore.put(i, i * 100);
  }

  std::vector<std::pair<uint32_t, uint32_t>> result = kvstore.scan(3, 7);
  assert(result.size() == 5);
  for (unsigned int i = 0; i < 5; i++) {
    assert(result[i].first == i + 3);
    assert(result[i].second == (i + 3) * 100);
  }

  // Scan on memtable and SSTs
  KVStore kvstore2(8, 2, 16);
  kvstore2.open("tests/test_db_6");
  for (int i = 1; i <= 100; i++) {
    kvstore2.put(i, i * 100);
  }

  int scan_start = 3;
  int scan_end = 96;
  unsigned int scan_range = scan_end - scan_start + 1;
  std::vector<std::pair<uint32_t, uint32_t>> result2 =
      kvstore2.scan(scan_start, scan_end);
  assert(result2.size() == scan_range);
  for (unsigned int i = 0; i < scan_range; i++) {
    // printf("%d %d\n", result2[i].first, result2[i].second);
    assert(result2[i].first == i + scan_start);
    assert(result2[i].second == (i + scan_start) * 100);
  }

  std::cout << "test_scan passed!" << std::endl;
}

int main() {
  Utils::clear_databases("tests", "test_db_");

  test_get_and_put();
  test_scan();

  Utils::clear_databases("tests", "test_db_");

  std::cout << GREEN << "All tests passed!\n" << RESET << std::endl;

  return 0;
}
