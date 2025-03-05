#pragma once

#include <stddef.h>
#include <stdint.h>

// hashtable node
// should be embedded into the data (intrusive data structure)
struct HNode {
  HNode *next = NULL;

  // store hash to avoid recalculation
  uint64_t hcode = 0;
};

// simple hashtable with chaining
struct HTab {
  HNode **tab = NULL; // array of slots
  size_t mask = 0;    // power of 2 array size, 2^n - 1
  size_t size = 0;    // number of keys
};

// the real hashtable interface.
// progressively move elements from older to newer table when resizing
// this will avoid huge latency spikes when resizing
// but this will make access/insertion slower due to 2 tables
struct HMap {
  HTab newer;
  HTab older;
  size_t migrate_pos = 0;
};

// lookup a key in the hashtable
HNode *hm_lookup(HMap *hmap, HNode *key, bool (*eq)(HNode *, HNode *));

// insert a key in the hashtable
void hm_insert(HMap *hmap, HNode *node);

// delete a key from the hashtable
HNode *hm_delete(HMap *hmap, HNode *key, bool (*eq)(HNode *, HNode *));

// clear the hashtable
void hm_clear(HMap *hmap);

// get the size of the hashtable
size_t hm_size(HMap *hmap);

// invoke the callback on each node until it returns false
void hm_foreach(HMap *hmap, bool (*f)(HNode *, void *), void *arg);
