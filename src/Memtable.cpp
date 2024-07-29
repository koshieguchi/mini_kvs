#include "memtable.h"

// Memtable constructor
Memtable::Memtable(int max_size) {
    this->max_size = max_size;
    this->red_black_tree = new RedBlackTree();
}

Memtable::~Memtable() { delete this->red_black_tree; }

bool Memtable::Put(uint64_t key, uint64_t value) {
    if (this->GetCurrentSize() + 1 > this->max_size) {
        return false;
    }
    this->red_black_tree->Insert(key, value);
    return true;
}

uint64_t Memtable::Get(uint64_t key) { return this->red_black_tree->Search(key); }

std::vector<DataEntry_t> Memtable::Scan(uint64_t key1, uint64_t key2) {
    std::vector<DataEntry_t> nodes_list;
    RedBlackTree::InorderTraversal(this->red_black_tree->GetRoot(), key1, key2, nodes_list);
    return nodes_list;
}

int Memtable::GetMaxSize() const { return this->max_size; }

int Memtable::GetCurrentSize() { return this->red_black_tree->GetCurrentSize(); }

std::vector<DataEntry_t> Memtable::GetAllData() {
    return this->Scan(this->red_black_tree->GetMinKey(), this->red_black_tree->GetMaxKey());
}

void Memtable::Reset() {
    delete this->red_black_tree;
    this->red_black_tree = new RedBlackTree();
}
