#include "timer.h"
#include "connection_manager.h"
#include "global_state.h"
#include "shared.h"
#include "storage.h"
#include <cassert>
#include <ctime>

uint64_t get_monotonic_msec() {
  struct timespec tv = {0, 0};
  clock_gettime(CLOCK_MONOTONIC, &tv);
  return uint64_t(tv.tv_sec) * 1000 + tv.tv_nsec / 1000 / 1000;
}
const uint64_t k_idle_timeout_ms = 5 * 1000;

uint32_t next_timer_ms() {
  uint64_t now_ms = get_monotonic_msec();
  uint64_t next_ms = (uint64_t)-1;
  // idle timers using a linked list
  if (!dlist_empty(&g_data.idle_list)) {
    Conn *conn = container_of(g_data.idle_list.next, Conn, idle_node);
    next_ms = conn->last_active_ms + k_idle_timeout_ms;
  }
  // TTL timers using a heap
  if (!g_data.heap.empty() && g_data.heap[0].val < next_ms) {
    next_ms = g_data.heap[0].val;
  }
  // timeout value
  if (next_ms == (uint64_t)-1) {
    return -1; // no timers, no timeouts
  }
  if (next_ms <= now_ms) {
    return 0; // missed?
  }
  return (int32_t)(next_ms - now_ms);
}

static bool hnode_same(HNode *node, HNode *key) { return node == key; }

void process_timers() {
  uint64_t now_ms = get_monotonic_msec();
  // idle timers using a linked list
  while (!dlist_empty(&g_data.idle_list)) {
    Conn *conn = container_of(g_data.idle_list.next, Conn, idle_node);
    uint64_t next_ms = conn->last_active_ms + k_idle_timeout_ms;
    if (next_ms >= now_ms) {
      break; // not expired
    }
    conn_destroy(conn);
  }
  // TTL timers using a heap
  const size_t k_max_works = 2000;
  size_t nworks = 0;
  const std::vector<HeapItem> &heap = g_data.heap;
  while (!heap.empty() && heap[0].val < now_ms) {
    Entry *ent = container_of(heap[0].ref, Entry, heap_idx);
    HNode *node = hm_delete(&g_data.db, &ent->node, &hnode_same);
    assert(node == &ent->node);
    // fprintf(stderr, "key expired: %s\n", ent->key.c_str());
    // delete the key
    entry_del(ent);
    if (nworks++ >= k_max_works) {
      // don't stall the server if too many keys are expiring at once
      break;
    }
  }
}
