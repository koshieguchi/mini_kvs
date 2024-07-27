#include "avl_tree.hpp"
#include "utils.hpp"
#include <algorithm>

int AVLTree::get_height(Node *node) {
  if (!node) {
    return 0;
  }
  return node->height;
}

int AVLTree::get_balance(Node *node) {
  if (!node) {
    return 0;
  }
  return get_height(node->left) - get_height(node->right);
}

Node *AVLTree::right_rotate(Node *node) {
  Node *left = node->left;
  Node *left_right = left->right;

  left->right = node;
  node->left = left_right;

  node->height = std::max(get_height(node->left), get_height(node->right)) + 1;
  left->height = std::max(get_height(left->left), get_height(left->right)) + 1;

  return left;
}

Node *AVLTree::left_rotate(Node *node) {
  Node *right = node->right;
  Node *right_left = right->left;

  right->left = node;
  node->right = right_left;

  node->height = std::max(get_height(node->left), get_height(node->right)) + 1;
  right->height =
      std::max(get_height(right->left), get_height(right->right)) + 1;

  return right;
}

Node *AVLTree::insert_node(Node *node, uint32_t key, uint32_t value) {
  if (!node) {
    return new Node(key, value);
  }

  if (key < node->key) {
    node->left = insert_node(node->left, key, value);
  } else if (key > node->key) {
    node->right = insert_node(node->right, key, value);
  } else {
    node->value = value;
    return node;
  }

  node->height = std::max(get_height(node->left), get_height(node->right)) + 1;

  int balance = get_balance(node);

  if (balance > 1 && key < node->left->key) {
    return right_rotate(node);
  }

  if (balance < -1 && key > node->right->key) {
    return left_rotate(node);
  }

  if (balance > 1 && key > node->left->key) {
    node->left = left_rotate(node->left);
    return right_rotate(node);
  }

  if (balance < -1 && key < node->right->key) {
    node->right = right_rotate(node->right);
    return left_rotate(node);
  }

  return node;
}

Node *AVLTree::get_node(Node *node, uint32_t key) {
  while (node != nullptr) {
    if (key < node->key) {
      node = node->left;
    } else if (key > node->key) {
      node = node->right;
    } else {
      return node;
    }
  }
  return nullptr;
}

/* Implementation for memtable */

void AVLTree::put(uint32_t key, uint32_t value) { root = insert_node(root, key, value); }

uint32_t AVLTree::get(uint32_t key) {
  Node *node = get_node(root, key);
  if (node) {
    return node->value;
  }
  return Utils::INVALID_VALUE;
}

void AVLTree::clear_recursive(Node *node) {
  if (node) {
    clear_recursive(node->left);
    clear_recursive(node->right);
    delete node;
  }
  root = nullptr;
}

void AVLTree::clear() { clear_recursive(root); }
