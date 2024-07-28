#include "memtable.h"

// Memtable constructor
Memtable::Memtable(int maxSize) {
    this->maxSize = maxSize;
    this->redBlackTree = new RedBlackTree();
}

Memtable::~Memtable() { delete this->redBlackTree; }

bool Memtable::Put(uint64_t key, uint64_t value) {
    if (this->GetCurrentSize() + 1 > this->maxSize) {
        return false;
    }
    this->redBlackTree->Insert(key, value);
    return true;
}

uint64_t Memtable::Get(uint64_t key) { return this->redBlackTree->Search(key); }

std::vector<DataEntry_t> Memtable::Scan(uint64_t key1, uint64_t key2) {
    std::vector<DataEntry_t> nodesList;
    RedBlackTree::InorderTraversal(this->redBlackTree->GetRoot(), key1, key2, nodesList);
    return nodesList;
}

int Memtable::GetMaxSize() const { return this->maxSize; }

int Memtable::GetCurrentSize() { return this->redBlackTree->GetCurrentSize(); }

std::vector<DataEntry_t> Memtable::GetAllData() {
    return this->Scan(this->redBlackTree->GetMinKey(), this->redBlackTree->GetMaxKey());
}

void Memtable::Reset() {
    delete this->redBlackTree;
    this->redBlackTree = new RedBlackTree();
}
