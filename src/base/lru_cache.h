#ifndef __BABEL_BASE_LRU_CACHE_H__
#define __BABEL_BASE_LRU_CACHE_H__

#include <map>

#include "base/debug.h"

namespace babel {
namespace {

template<typename T>
struct CircularListNode {
  CircularListNode(T t) : key(t) {}

  const T key;
  CircularListNode<T>* prev;
  CircularListNode<T>* next;
};

}  // namespace

template<typename K,typename V>
class LRUCache {
 public:
  LRUCache(int capacity) : capacity_(capacity), head_(nullptr) {
    ASSERT(capacity_ > 0);
  }

  ~LRUCache() {
    for (auto& pair : values_) {
      delete pair.second;
      delete priorities_.at(pair.first);
    }
  }

  // Returns nullptr if the key has been evicted. This method is non-const
  // because it updates the key's priority.
  V* Get(K key) {
    if (values_.find(key) == values_.end()) {
      return nullptr;
    }
    CircularListNode<K>* node = priorities_.at(key);
    Remove(node);
    Insert(node);
    return values_.at(key);
  }

  // This method will crash if the value is null or if the key is already
  // present in the crash. The cache takes ownership of the value pointer.
  void Set(K key, V* value) {
    ASSERT(values_.find(key) == values_.end());
    ASSERT(value != nullptr);
    CircularListNode<K>* node = new CircularListNode<K>(key);
    values_[key] = value;
    priorities_[key] = node;
    Insert(node);

    if (values_.size() > capacity_) {
      CircularListNode<K>* back = head_->prev;
      const K& evicted = back->key;
      delete values_.at(evicted);
      values_.erase(evicted);
      priorities_.erase(evicted);
      Remove(back);
      delete back;
    }
  }

 private:
  // All insertions happen at the head of the list.
  void Insert(CircularListNode<K>* node) {
    if (head_ == nullptr) {
      node->prev = node;
      node->next = node;
    } else {
      node->prev = head_->prev;
      node->next = head_;
      node->prev->next = node;
      head_->prev = node;
    }
    head_ = node;
  }

  // The caller owns the removed node. They may destruct it or re-insert it.
  void Remove(CircularListNode<K>* node) {
    node->prev->next = node->next;
    node->next->prev = node->prev;
    if (node == head_) {
      head_ = (node == node->next ? nullptr : node->next);
    }
  }

  const int capacity_;
  std::map<K,V*> values_;
  std::map<K,CircularListNode<K>*> priorities_;
  CircularListNode<K>* head_;
};

}  // namespace std

#endif  // __BABEL_BASE_LRU_CACHE_H__
