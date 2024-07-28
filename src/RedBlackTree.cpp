#include "RedBlackTree.h"
#include <iostream>
#include <queue>

RedBlackTree::RedBlackTree(Node *root) {
    this->root = root;
    this->currentSize = 0;
    this->maxKey = 0;
    this->minKey = std::numeric_limits<uint64_t>::max();
}

RedBlackTree::~RedBlackTree() {
    delete this->root;
}

Node *RedBlackTree::GetRoot() {
    return this->root;
}

int RedBlackTree::GetCurrentSize() const {
    return this->currentSize;
}

void RedBlackTree::SetRoot(Node *newRoot) {
    this->root = newRoot;
}

uint64_t RedBlackTree::GetMaxKey() const {
    return this->maxKey;
}

uint64_t RedBlackTree::GetMinKey() const {
    return this->minKey;
}

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

void RedBlackTree::ConnectParentWithNewChild(Node *parent, Node *oldChild, Node *newChild) {

    if (parent == nullptr) {
        this->SetRoot(newChild);
    } else if (oldChild == parent->GetRightChild()) {
        parent->SetRightChild(newChild);
    } else if (oldChild == parent->GetLeftChild()) {
        parent->SetLeftChild(newChild);
    } else {
        std::cout << "The node is not a child of parent." << std::endl;
        return;
    }

    if (newChild != nullptr) {
        newChild->SetParent(parent);
    }
}

void RedBlackTree::RotateRight(Node *node) {

    Node *parent = node->GetParent();
    Node *leftChild = node->GetLeftChild();

    // Rotate right around the node 
    node->SetLeftChild(leftChild->GetRightChild());
    if (leftChild->GetRightChild() != nullptr) {
        leftChild->GetRightChild()->SetParent(node);
    }

    leftChild->SetRightChild(node);
    node->SetParent(leftChild);

    ConnectParentWithNewChild(parent, node, leftChild);
}

void RedBlackTree::RotateLeft(Node *node) {

    Node *parent = node->GetParent();
    Node *rightChild = node->GetRightChild();

    // Rotate left around the node 
    node->SetRightChild(rightChild->GetLeftChild());
    if (rightChild->GetLeftChild() != nullptr) {
        rightChild->GetLeftChild()->SetParent(node);
    }

    rightChild->SetLeftChild(node);
    node->SetParent(rightChild);

    ConnectParentWithNewChild(parent, node, rightChild);
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
    Node *newNode = new Node(key, value, nullptr, nullptr, parent, RED);

    if (parent == nullptr) {
        this->root = newNode;
    } else if (key > parent->GetKey()) {
        parent->SetRightChild(newNode);
    } else {
        parent->SetLeftChild(newNode);
    }

    // Make sure the properties of Red-Black tree remains satisfied
    this->MaintainRedBlackTreePropertiesAfterInsert(newNode);
    this->currentSize++;
    if (key > this->maxKey) {
        this->maxKey = key;
    }
    if (key < this->minKey) {
        this->minKey = key;
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

    Node *grandParent = parent->GetParent();
    Node *uncle;
    if (parent == grandParent->GetRightChild()) {
        uncle = grandParent->GetLeftChild();
    } else if (parent == grandParent->GetLeftChild()) {
        uncle = grandParent->GetRightChild();
    } else {
        // throw an error "parent is not a child of the grandParent"
        std::cout << "The parent is not a child of the grandParent." << std::endl;
        return;
    }

    // Case 3: The parent and the uncle nodes are both red.
    // We have to set them to black and then set the grandparent red.
    // Then we have to recursively repair upwards to make sure no two
    // red nodes are placed beside each other.
    if (uncle != nullptr && uncle->GetColor() == RED) {
        parent->SetColor(BLACK);
        uncle->SetColor(BLACK);
        grandParent->SetColor(RED);

        // The grandparent is now red, but the grand-grandparent can also be red,
        // in which case we should recursively call this method on the grandParent
        // to make sure we maintain the property that no two red nodes are located
        // beside each other.
        MaintainRedBlackTreePropertiesAfterInsert(grandParent);
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
    else if (parent == grandParent->GetRightChild()) {
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
        RotateLeft(grandParent);

        parent->SetColor(BLACK); // Black
        grandParent->SetColor(RED); // Red
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
        RotateRight(grandParent);

        parent->SetColor(BLACK); // Black
        grandParent->SetColor(RED); // Red
    }
}

void RedBlackTree::InorderTraversal(Node *node, uint64_t key1, uint64_t key2, std::vector<DataEntry_t> &nodesList) {
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
        InorderTraversal(node->GetLeftChild(), key1, key2, nodesList);
    }

    if (node->GetKey() >= key1 && node->GetKey() <= key2) {
        RedBlackTree::Visit(node, nodesList);
    }

    // Only visit the right subtree if the current node's key is less than key2.
    // That is because the right subtree of the current node refers to keys greater
    // than key2, and we do not want them.
    if (node->GetKey() < key2) {
        InorderTraversal(node->GetRightChild(), key1, key2, nodesList);
    }
}

void RedBlackTree::Visit(Node *node, std::vector<DataEntry_t> &nodesList) {
    DataEntry_t data = std::make_pair(node->GetKey(), node->GetValue());
    nodesList.push_back(data);
}
