#include "kv_store.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>

#include "utils.hpp"
#include "utils/test_utils.hpp"
#include "xxhash.h"

/*
 * Test Basic API
 */

void test_open() {
  KVStore kvstore(10, 2, 4);
  kvstore.open("tests/test_db_1");

  assert(fs::exists(fs::current_path() / fs::path("tests/test_db_1")));
  std::cout << "test_open passed!" << std::endl;
}

void test_put() {
  KVStore kvstore(2, 2, 4);
  kvstore.open("tests/test_db_2");

  kvstore.put(1, 100);
  assert(kvstore.get(1) == 100);

  kvstore.put(2, 200);
  assert(kvstore.get(2) == 200);

  kvstore.put(3, 300);
  // At this point key 1 should be flushed to SST
  // Get should also search in SSTs

  assert(kvstore.get(1) == 100);
  assert(kvstore.get(3) == 300);

  // test updates
  kvstore.put(2, 0);
  kvstore.put(4, 400);
  kvstore.put(5, 500);
  assert(kvstore.get(2) == 0);

  std::cout << "test_put passed!" << std::endl;
}

void test_get() {
  KVStore kvstore(2, 2, 4);
  kvstore.open(fs::path("tests/test_db_3"));

  kvstore.put(1, 100);
  kvstore.put(2, 200);
  kvstore.put(3, 300);
  kvstore.put(4, 400);

  assert(kvstore.get(1) == 100);
  assert(kvstore.get(4) == 400);
  assert(kvstore.get(0) == Utils::INVALID_VALUE);
  assert(kvstore.get(5) == Utils::INVALID_VALUE);

  kvstore.delete_key(1);
  kvstore.put(5, 500);
  assert(kvstore.get(1) == Utils::INVALID_VALUE);
  assert(kvstore.get(5) == 500);
  std::cout << "test_get!" << std::endl;
}

void test_scan() {
  // Scan only on memtable
  KVStore kvstore(10, 2, 4);
  kvstore.open("tests/test_db_4");

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
  KVStore kvstore2(2, 2, 4);
  kvstore2.open("tests/test_db_5");

  for (int i = 1; i <= 9; i++) {
    kvstore2.put(i, i * 100);
  }

  std::vector<std::pair<uint32_t, uint32_t>> result2 = kvstore2.scan(3, 7);
  assert(result2.size() == 5);
  for (unsigned int i = 0; i < 5; i++) {
    assert(result2[i].first == i + 3);
    assert(result2[i].second == (i + 3) * 100);
  }

  std::cout << "test_scan passed!" << std::endl;
}

void test_delete() {
  KVStore kvstore(2, 2, 4);
  kvstore.open("tests/test_db_8");

  kvstore.put(1, 100);
  kvstore.put(2, 200);
  kvstore.put(3, 300);
  kvstore.put(4, 400);

  kvstore.put(5, 500);
  kvstore.put(6, 600);
  kvstore.put(7, 700);
  kvstore.delete_key(1);
  kvstore.put(8, 800);
  kvstore.put(9, 900);

  kvstore.put(2, 200);

  kvstore.delete_key(9);

  assert(kvstore.get(9) == Utils::INVALID_VALUE);
  assert(kvstore.get(1) == Utils::INVALID_VALUE);
  assert(kvstore.get(2) == 200);

  std::cout << "test_delete passed!" << std::endl;
}

// Integration test for lsm compaction
void test_lsm_tree_compaction() {
  KVStore kvstore(2, 2, 4);
  kvstore.open("tests/test_db_6");

  kvstore.put(1, 100);
  kvstore.put(2, 200);
  // no compaction
  assert(
      fs::exists(fs::current_path() / fs::path("tests/test_db_6/sst_0.dat")));
  assert(
      !fs::exists(fs::current_path() / fs::path("tests/test_db_6/sst_1.dat")));

  kvstore.put(3, 300);
  kvstore.put(4, 400);
  // compaction is triggered
  assert(
      !fs::exists(fs::current_path() / fs::path("tests/test_db_6/sst_0.dat")));
  assert(
      !fs::exists(fs::current_path() / fs::path("tests/test_db_6/sst_1.dat")));
  assert(
      fs::exists(fs::current_path() / fs::path("tests/test_db_6/sst_2.dat")));

  for (int i = 5; i <= 8; i++) {
    kvstore.put(i, i * 100);
  }
  // compaction is triggered again
  for (int i = 0; i < 4; i++) {
    assert(!fs::exists(
        fs::current_path() /
        (fs::path("tests/test_db_6/sst_" + std::to_string(i) + ".dat"))));
  }
  assert(
      fs::exists(fs::current_path() / fs::path("tests/test_db_6/sst_4.dat")));

  std::cout << "test_lsm_tree_compaction!" << std::endl;
}

void test_close() {
  KVStore kvstore(3, 2, 4);
  kvstore.open("tests/test_db_7");

  kvstore.put(4, 400);
  kvstore.put(1, 100);
  kvstore.close();

  kvstore.put(3, 300);
  kvstore.put(2, 200);
  kvstore.close();

  assert(
      fs::exists(fs::current_path() / fs::path("tests/test_db_7/sst_2.dat")));

  // Reopen the db and check if the values are still there
  KVStore kvstore2(10, 2, 4);
  kvstore2.open("tests/test_db_7");
  assert(kvstore2.get(1) != 400);
  assert(kvstore2.get(3) == 300);
  assert(kvstore2.get(2) == 200);

  std::cout << "test_close passed!" << std::endl;
}

int main() {
  Utils::clear_databases("tests", "test_db_");

  test_open();
  test_get();
  test_put();
  test_scan();
  test_delete();
  test_lsm_tree_compaction();
  test_close();

  Utils::clear_databases("tests", "test_db_");

  std::cout << GREEN << "All tests passed!\n" << RESET << std::endl;

  return 0;
}
