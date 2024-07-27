#include "utils/test_utils.hpp"
#include <cassert>
#include <iostream>
#include <cstdint>

void test_equal() {
  AVLTree tree1;
  tree1.put(1, 1);
  tree1.put(2, 2);
  tree1.put(3, 3);

  AVLTree tree2;
  tree2.put(1, 1);
  tree2.put(2, 2);
  tree2.put(3, 3);

  assert(are_identical_avl_trees(&tree1, &tree2) == true);
  std::cout << "test_equal passed!" << std::endl;
}

void test_not_equal() {
  AVLTree tree1;
  tree1.put(1, 1);

  AVLTree tree2;
  tree2.put(2, 0);

  assert(are_identical_avl_trees(&tree1, &tree2) == false);
  std::cout << "test_not_equal passed!" << std::endl;
}

void test_not_equal_same_key() {
  AVLTree tree1;
  tree1.put(1, 1);

  AVLTree tree2;
  tree2.put(1, 0);

  assert(are_identical_avl_trees(&tree1, &tree2) == false);
  std::cout << "test_not_equal_same_key passed!" << std::endl;
}

void test_not_equal_same_value() {
  AVLTree tree1;
  tree1.put(1, 1);

  AVLTree tree2;
  tree2.put(2, 1);

  assert(are_identical_avl_trees(&tree1, &tree2) == false);
  std::cout << "test_not_equal_same_value passed!" << std::endl;
}

void test_equal_null() {
  assert(are_identical_avl_trees((AVLTree *)nullptr, (AVLTree *)nullptr) ==
         true);
  std::cout << "test_equal_null passed!" << std::endl;
}

void test_not_equal_null() {
  AVLTree tree1;
  assert(are_identical_avl_trees(&tree1, nullptr) == false);
  std::cout << "test_not_equal_null passed!" << std::endl;
}

void test_no_rotation() {
  AVLTree tree1;
  tree1.put(2, 0);
  tree1.put(3, 0);
  tree1.put(1, 0);

  Node node1(2, 0);
  Node node2(3, 0);
  Node node3(1, 0);
  node1.height = 2;
  node1.left = &node3;
  node1.right = &node2;
  AVLTree tree2;
  tree2.root = &node1;

  assert(are_identical_avl_trees(&tree1, &tree2) == true);
  std::cout << "test_no_rotation passed!" << std::endl;
}

void test_rotation() {
  AVLTree tree1;
  tree1.put(3, 0);
  tree1.put(2, 0);
  tree1.put(1, 0);

  Node node1(2, 0);
  Node node2(3, 0);
  Node node3(1, 0);
  node1.height = 2;
  node1.left = &node3;
  node1.right = &node2;
  AVLTree tree2;
  tree2.root = &node1;

  assert(are_identical_avl_trees(&tree1, &tree2) == true);
  std::cout << "test_rotation passed!" << std::endl;
}

int main() {
  test_equal();
  test_not_equal();
  test_not_equal_same_key();
  test_not_equal_same_value();
  test_no_rotation();
  test_rotation();

  std::cout << GREEN << "All tests passed!\n" << RESET << std::endl;

  return 0;
}
