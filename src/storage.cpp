#include "storage.h"
#include "global_state.h"
#include "shared.h"
#include "timer.h"

// Create a new entry
Entry *entry_new(uint32_t type) {
  Entry *ent = new Entry();
  ent->type = type;
  return ent;
}

// Entry deletion callback for thread pool
static void entry_del_func(void *arg) { entry_del_sync((Entry *)arg); }

// Synchronous entry deletion
void entry_del_sync(Entry *ent) {
  if (ent->type == T_ZSET) {
    zset_clear(&ent->zset);
  }
  delete ent;
}

// Delete an entry (might be asynchronous)
void entry_del(Entry *ent) {
  entry_set_ttl(ent, -1); // Remove from the heap data structure

  // Run the destructor in a thread pool for large data structures such as the
  // zset as deleting is O(n) operation
  size_t set_size = (ent->type == T_ZSET) ? hm_size(&ent->zset.hmap) : 0;
  const size_t k_large_container_size = 1000;

  if (set_size > k_large_container_size) {
    thread_pool_queue(&g_data.thread_pool, &entry_del_func, ent);
  } else {
    entry_del_sync(ent);
  }
}

// Set or remove TTL for an entry
void entry_set_ttl(Entry *ent, int64_t ttl_ms) {
  if (ttl_ms < 0 && ent->heap_idx != (size_t)-1) {
    // Setting a negative TTL means removing the TTL
    heap_delete(g_data.heap, ent->heap_idx);
    ent->heap_idx = -1;
  } else if (ttl_ms >= 0) {
    // Add or update the heap data structure
    uint64_t expire_at = get_monotonic_msec() + (uint64_t)ttl_ms;
    HeapItem item = {expire_at, &ent->heap_idx};
    heap_upsert(g_data.heap, ent->heap_idx, item);
  }
}

// Equality comparison for the top-level hashtable
bool entry_eq(HNode *node, HNode *key) {
  struct Entry *ent = container_of(node, struct Entry, node);
  struct LookupKey *keydata = container_of(key, struct LookupKey, node);
  return ent->key == keydata->key;
}

// Delete an item from the heap
void heap_delete(std::vector<HeapItem> &a, size_t pos) {
  // Swap the erased item with the last item
  a[pos] = a.back();
  a.pop_back();
  // Update the swapped item
  if (pos < a.size()) {
    heap_update(a.data(), pos, a.size());
  }
}

// Insert or update an item in the heap
void heap_upsert(std::vector<HeapItem> &a, size_t pos, HeapItem t) {
  if (pos < a.size()) {
    a[pos] = t; // Update an existing item
  } else {
    pos = a.size();
    a.push_back(t); // Or add a new item
  }
  heap_update(a.data(), pos, a.size());
}

// Empty ZSet for comparison
static const ZSet k_empty_zset;

// Get a ZSet from the database
ZSet *expect_zset(std::string &s) {
  LookupKey key;
  key.key.swap(s);
  key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());

  HNode *hnode = hm_lookup(&g_data.db, &key.node, &entry_eq);
  if (!hnode) { // A non-existent key is treated as an empty zset
    return (ZSet *)&k_empty_zset;
  }

  Entry *ent = container_of(hnode, Entry, node);
  return ent->type == T_ZSET ? &ent->zset : NULL;
}
