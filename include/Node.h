#ifndef NODE_H
#define NODE_H

#include <cstdint>

enum Color { RED = 1, BLACK = 0 };

/**
 * Class representing a node in the Red-Black Tree data structure.
 */
class Node {
   private:
    /* data */
    uint64_t key;
    uint64_t value;
    Node *left;
    Node *right;
    Node *parent;  // Used in Red-Black tree
    Color color;   // Used in Red-Black tree
   public:
    Node(uint64_t key, uint64_t value, Node *left, Node *right, Node *parent, Color color)
        : key(key), value(value), left(left), right(right), parent(parent), color(color) {};

    ~Node() {
        delete this->left;
        delete this->right;
    }

    [[nodiscard]] uint64_t GetKey() const { return this->key; }

    [[nodiscard]] uint64_t GetValue() const { return this->value; }

    Node *GetLeftChild() { return this->left; }

    Node *GetRightChild() { return this->right; }

    Node *GetParent() { return this->parent; }

    bool GetColor() { return this->color; }

    void SetLeftChild(Node *new_left) { this->left = new_left; }

    void SetRightChild(Node *new_right) { this->right = new_right; }

    void SetParent(Node *new_parent) { this->parent = new_parent; }

    void SetColor(Color new_color) { this->color = new_color; }
};

#endif  // NODE_H
