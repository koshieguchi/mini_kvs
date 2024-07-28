#include "avl_tree.hpp"

bool are_identical_trees(Node *node1, Node *node2) {
    if (node1 == nullptr && node2 == nullptr) {
        return true;
    }

    if (node1 == nullptr || node2 == nullptr) {
        return false;
    }

    if ((node1->key != node2->key || node1->value != node2->value) || node1->height != node2->height) {
        return false;
    }

    return node1->key == node2->key && are_identical_trees(node1->left, node2->left) &&
           are_identical_trees(node1->right, node2->right);
}

bool are_identical_avl_trees(AVLTree *tree1, AVLTree *tree2) { return are_identical_trees(tree1->root, tree2->root); }
