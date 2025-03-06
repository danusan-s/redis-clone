#ifndef STORAGE_H
#define STORAGE_H

#include "hashtable.h"
#include "heap.h"
#include "zset.h"
#include <string>
#include <vector>

// Value types
enum {
  T_INIT = 0,
  T_STR = 1,  // String
  T_ZSET = 2, // Sorted set
};

// KV pair for the top-level hashtable
struct Entry {
  struct HNode node;    // Hashtable node
  std::string key;      // Key to lookup
  size_t heap_idx = -1; // TTL heap index
  uint32_t type = 0;    // Whether string or sorted set
  std::string str;
  ZSet zset;
};

// Key lookup struct
struct LookupKey {
  struct HNode node; // Hashtable node
  std::string key;
};

// Entry operations
Entry *entry_new(uint32_t type);
void entry_del_sync(Entry *ent);
void entry_del(Entry *ent);
void entry_set_ttl(Entry *ent, int64_t ttl_ms);

// Comparison function for the hashtable
bool entry_eq(HNode *node, HNode *key);

// Heap operations
void heap_delete(std::vector<HeapItem> &a, size_t pos);
void heap_upsert(std::vector<HeapItem> &a, size_t pos, HeapItem t);

// ZSet utilities
ZSet *expect_zset(std::string &s);

#endif // STORAGE_H
