#ifndef RED_BLACK_TREE_H
#define RED_BLACK_TREE_H

#include <limits>
#include <utility>
#include <vector>

#include "Node.h"
#include "utils.h"

/**
 * Class representing a Red-Black Tree data structure.
 */
class RedBlackTree {
   private:
    Node *root;
    int current_size;
    uint64_t maxKey;
    uint64_t min_key;

    void RotateRight(Node *node);

    void RotateLeft(Node *node);

    void ConnectParentWithNewChild(Node *parent, Node *old_child, Node *new_child);

    void MaintainRedBlackTreePropertiesAfterInsert(Node *node);

    void SetRoot(Node *root);

    static void Visit(Node *node, std::vector<DataEntry_t> &nodes_list);

   public:
    explicit RedBlackTree(Node *root = nullptr);

    ~RedBlackTree();

    /**
     * Get the root of the tree.
     */
    Node *GetRoot();

    /**
     * Get the current size of the tree.
     */
    [[nodiscard]] int GetCurrentSize() const;

    /**
     * Get the key with the largest value currently in the tree.
     */
    [[nodiscard]] uint64_t GetMaxKey() const;

    /**
     * Get the key with the smallest value currently in the tree.
     */
    [[nodiscard]] uint64_t GetMinKey() const;

    /**
     * Searches for value associated with given key.
     *
     * @param key
     * @return the value associated with the key.
     */
    uint64_t Search(uint64_t key);

    /**
     * Insert a new key-value pair into the tree.
     *
     * @param key
     * @param value
     */
    void Insert(uint64_t key, uint64_t value);

    /**
     * Traverse the tree rooted at given <node> and gather all key-value pairs whose
     * key is within the range of [key1, key2].
     *
     * @param node the root of the tree.
     * @param key1 the lower bound of the scan range.
     * @param key2 the upper bound of the scan range.
     * @param nodes_list the vector to place resulting key-value pairs in.
     */
    static void InorderTraversal(Node *node, uint64_t key1, uint64_t key2, std::vector<DataEntry_t> &nodes_list);
};

#endif  // RED_BLACK_TREE_H
