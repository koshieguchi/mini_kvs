#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include <avl_tree.hpp>

constexpr const char *GREEN = "\033[32m";
constexpr const char *RESET = "\033[0m";

bool are_identical_avl_trees(AVLTree *tree1, AVLTree *tree2);
bool are_identical_trees(Node *node1, Node *node2);

#endif  // TEST_UTILS_H
