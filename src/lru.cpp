#include "lru.hpp"

#include <string>

void LRU::insert(std::string key) {
  LRUNode* node = new LRUNode(key);

  if (front != nullptr) {
    front->prev = node;
    node->next = front;
  }
  front = node;

  if (rear == nullptr) {
    rear = node;
  }

  node_map[key] = node;
}

void LRU::remove(std::string key) {
  if (node_map.find(key) == node_map.end()) {
    // not found
  } else {
    LRUNode* node = node_map[key];
    if (node->prev == nullptr) {
      // this node is at the front
      front = node->next;
    } else {
      node->prev->next = node->next;
    }
    if (node->next == nullptr) {
      // this node is at the rear
      rear = node->prev;
    } else {
      node->next->prev = node->prev;
    }
    node_map.erase(key);
  }
}

void LRU::update(std::string key) {
  LRUNode* node = node_map[key];
  if (node != front) {
    if (node->prev != nullptr) {
      if (node == rear) {
        rear = node->prev;
      }
      node->prev->next = node->next;
    }
    if (node->next != nullptr) {
      node->next->prev = node->prev;
    }
    front->prev = node;
    node->next = front;
    front = node;
  }
}

std::string LRU::evict() {
  LRUNode* node = rear;
  if (rear->prev == nullptr) {
    // then the linked list only has one node
    rear = nullptr;
    front = nullptr;
  } else {
    rear->prev->next = nullptr;
    rear = rear->prev;
  }
  node_map.erase(node->key);
  return node->key;
}