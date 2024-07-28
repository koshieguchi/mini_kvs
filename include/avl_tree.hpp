#ifndef AVL_TREE_HPP_
#define AVL_TREE_HPP_

#include <cstdint>

class Node {
public:
  uint32_t key;
  uint32_t value;
  int height;
  Node *left;
  Node *right;

  Node(uint32_t key, uint32_t value)
      : key(key), value(value), height(1), left(nullptr), right(nullptr) {}
};

// Balanced binary search tree for memtable
class AVLTree {
private:
  int get_height(Node *node);
  int get_balance(Node *node);
  Node *right_rotate(Node *node);
  Node *left_rotate(Node *node);
  Node *insert_node(Node *node, uint32_t key, uint32_t value);
  Node *get_node(Node *node, uint32_t key);

public:
  Node *root;
  AVLTree() : root(nullptr) {}
  void put(uint32_t key, uint32_t value);
  uint32_t get(uint32_t key);
  void clear_recursive(Node *node);
  void clear();
};

#endif // AVL_TREE_HPP_
