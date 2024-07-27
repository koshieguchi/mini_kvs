#ifndef LRU_H
#define LRU_H

#include <string>
#include <unordered_map>

class LRUNode {
 public:
  LRUNode *prev;
  LRUNode *next;
  std::string key;

  LRUNode(std::string key) : prev(nullptr), next(nullptr), key(key) {}
};

class LRU {
 private:
  std::unordered_map<std::string, LRUNode *> node_map;

 public:
  LRUNode *front;  // Most recently accessed key
  LRUNode *rear;   // Least recently used

  LRU() : node_map(), front(nullptr), rear(nullptr) {}

  void insert(std::string key);
  void remove(std::string key);
  void update(std::string key);  // Move key to the front of the linked list
  std::string evict();  // Evict from linked list and return evicted key
};

#endif  // LRU_H
