#include "red_black_tree.h"

#include <iostream>
#include <queue>

RedBlackTree::RedBlackTree(Node *root) {
    this->root = root;
    this->current_size = 0;
    this->maxKey = 0;
    this->min_key = std::numeric_limits<uint64_t>::max();
}

RedBlackTree::~RedBlackTree() { delete this->root; }

Node *RedBlackTree::GetRoot() { return this->root; }

int RedBlackTree::GetCurrentSize() const { return this->current_size; }

void RedBlackTree::SetRoot(Node *newRoot) { this->root = newRoot; }

uint64_t RedBlackTree::GetMaxKey() const { return this->maxKey; }

uint64_t RedBlackTree::GetMinKey() const { return this->min_key; }

uint64_t RedBlackTree::Search(uint64_t key) {
    Node *node = this->root;
    while (node != nullptr) {
        if (key == node->GetKey()) {
            return node->GetValue();
        } else if (key > node->GetKey()) {
            node = node->GetRightChild();
        } else {
            node = node->GetLeftChild();
        }
    }
    // key not found.
    return Utils::INVALID_VALUE;
}

void RedBlackTree::ConnectParentWithNewChild(Node *parent, Node *old_child, Node *new_child) {
    if (parent == nullptr) {
        this->SetRoot(new_child);
    } else if (old_child == parent->GetRightChild()) {
        parent->SetRightChild(new_child);
    } else if (old_child == parent->GetLeftChild()) {
        parent->SetLeftChild(new_child);
    } else {
        std::cout << "The node is not a child of parent." << std::endl;
        return;
    }

    if (new_child != nullptr) {
        new_child->SetParent(parent);
    }
}

void RedBlackTree::RotateRight(Node *node) {
    Node *parent = node->GetParent();
    Node *left_child = node->GetLeftChild();

    // Rotate right around the node
    node->SetLeftChild(left_child->GetRightChild());
    if (left_child->GetRightChild() != nullptr) {
        left_child->GetRightChild()->SetParent(node);
    }

    left_child->SetRightChild(node);
    node->SetParent(left_child);

    ConnectParentWithNewChild(parent, node, left_child);
}

void RedBlackTree::RotateLeft(Node *node) {
    Node *parent = node->GetParent();
    Node *right_child = node->GetRightChild();

    // Rotate left around the node
    node->SetRightChild(right_child->GetLeftChild());
    if (right_child->GetLeftChild() != nullptr) {
        right_child->GetLeftChild()->SetParent(node);
    }

    right_child->SetLeftChild(node);
    node->SetParent(right_child);

    ConnectParentWithNewChild(parent, node, right_child);
}

void RedBlackTree::Insert(uint64_t key, uint64_t value) {
    Node *node = this->root;
    Node *parent = nullptr;

    // Find the new position for this new node
    while (node != nullptr) {
        parent = node;
        if (key > node->GetKey()) {
            node = node->GetRightChild();
        } else if (key <= node->GetKey()) {
            node = node->GetLeftChild();
        }
    }

    // At this point the variable parent refers to parent of this new node
    // Color of the new node is red.
    Node *new_node = new Node(key, value, nullptr, nullptr, parent, RED);

    if (parent == nullptr) {
        this->root = new_node;
    } else if (key > parent->GetKey()) {
        parent->SetRightChild(new_node);
    } else {
        parent->SetLeftChild(new_node);
    }

    // Make sure the properties of Red-Black tree remains satisfied
    this->MaintainRedBlackTreePropertiesAfterInsert(new_node);
    this->current_size++;
    if (key > this->maxKey) {
        this->maxKey = key;
    }
    if (key < this->min_key) {
        this->min_key = key;
    }
}

void RedBlackTree::MaintainRedBlackTreePropertiesAfterInsert(Node *node) {
    Node *parent = node->GetParent();

    // Case 1: The new node is the root (i.e. parent is nullptr).
    if (parent == nullptr) {
        // Let the root node be always black
        node->SetColor(BLACK);
        return;
    }

    // Case 2: If the parent is black, all is ok since the newly added node was colored red.
    // The parent node being the root AND be red, will not happen since we always let the root node black.
    if (parent->GetColor() == BLACK) {
        return;
    }

    Node *grand_parent = parent->GetParent();
    Node *uncle;
    if (parent == grand_parent->GetRightChild()) {
        uncle = grand_parent->GetLeftChild();
    } else if (parent == grand_parent->GetLeftChild()) {
        uncle = grand_parent->GetRightChild();
    } else {
        // throw an error "parent is not a child of the grand_parent"
        std::cout << "The parent is not a child of the grand_parent." << std::endl;
        return;
    }

    // Case 3: The parent and the uncle nodes are both red.
    // We have to set them to black and then set the grandparent red.
    // Then we have to recursively repair upwards to make sure no two
    // red nodes are placed beside each other.
    if (uncle != nullptr && uncle->GetColor() == RED) {
        parent->SetColor(BLACK);
        uncle->SetColor(BLACK);
        grand_parent->SetColor(RED);

        // The grandparent is now red, but the grand-grandparent can also be red,
        // in which case we should recursively call this method on the grand_parent
        // to make sure we maintain the property that no two red nodes are located
        // beside each other.
        MaintainRedBlackTreePropertiesAfterInsert(grand_parent);
    }

    // Case 4: The parent is red, but the uncle is black,
    // (note the uncle can be a Nil node which is also considered black).
    // Also, the new node is an inner child of its parent and grandparent (i.e. a triangular shape).
    // In this case, do:
    // a) Rotate at the parent node, in the opposite direction of the new node with respect to parent.
    // b) Rotate at the grandparent node, in the opposite direction of the previous rotation in part a.

    // Case 5: The parent is red, but the uncle is black,
    // (note the uncle can be a Nil node which is also considered black).
    // But the new node is an outer child of its parent and grandparent (i.e. a straight line shape).
    // In this case we do:
    // Rotate at the grandparent node, in the opposite direction of the new node or parent, which
    // both are either "right" or "left".

    // The scenarios of cases 4-b and 5 overlap, so we just implement
    // case 4-a and 5 by mixing them as follows:
    else if (parent == grand_parent->GetRightChild()) {
        if (node == parent->GetLeftChild()) {
            // The new node is to the left of the parent,
            // so rotate in the opposite direction which is the right.
            RotateRight(parent);

            // Let the parent be the new node, since that is
            // now the root of the rotated subtree.
            parent = node;
        }

        // Case 5-a: The node refers to a right-right outer child
        // of the grandparent, so we have to do a rotation of
        // opposite direction (i.e. a left rotation) on the grandparent.
        RotateLeft(grand_parent);

        parent->SetColor(BLACK);      // Black
        grand_parent->SetColor(RED);  // Red
    } else {
        // The parent is the left child of the grandparent.
        // The new node is the right child of the parent,
        // so rotate in the opposite direction which is the left.
        if (node == parent->GetRightChild()) {
            RotateLeft(parent);

            // Let the parent be the new node, since that is
            // now the root of the rotated subtree.
            parent = node;
        }

        // Case 5-a: The node refers to a left-left outer child
        // of the grandparent, so we have to do a rotation of
        // opposite direction (i.e. a right rotation) on the grandparent.
        RotateRight(grand_parent);

        parent->SetColor(BLACK);      // Black
        grand_parent->SetColor(RED);  // Red
    }
}

void RedBlackTree::InorderTraversal(Node *node, uint64_t key1, uint64_t key2, std::vector<DataEntry_t> &nodes_list) {
    // Since the red-black tree is a balanced binary search tree, an inorder
    // traversal of that give us its elements in an ascending sorted order.
    if (node == nullptr) {
        return;
    }
    // Inorder traversal:
    // 1. Recursively traverse the current node's left subtree.
    // 2. Visit the current node (in the figure: position green).
    // 3. Recursively traverse the current node's right subtree.

    // Only visit the left subtree if the current node's key is greater than key1.
    // That is because the left subtree of the current node refers to keys smaller
    // than key1, and we do not want them.
    if (node->GetKey() > key1) {
        InorderTraversal(node->GetLeftChild(), key1, key2, nodes_list);
    }

    if (node->GetKey() >= key1 && node->GetKey() <= key2) {
        RedBlackTree::Visit(node, nodes_list);
    }

    // Only visit the right subtree if the current node's key is less than key2.
    // That is because the right subtree of the current node refers to keys greater
    // than key2, and we do not want them.
    if (node->GetKey() < key2) {
        InorderTraversal(node->GetRightChild(), key1, key2, nodes_list);
    }
}

void RedBlackTree::Visit(Node *node, std::vector<DataEntry_t> &nodes_list) {
    DataEntry_t data = std::make_pair(node->GetKey(), node->GetValue());
    nodes_list.push_back(data);
}
