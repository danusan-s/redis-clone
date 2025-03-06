#include "commands.h"
#include "global_state.h"
#include "protocol.h"
#include "shared.h"
#include "storage.h"
#include "timer.h"

#include <cmath>

void do_get(std::vector<std::string> &cmd, Buffer &out) {
  // a dummy struct just for the lookup
  LookupKey key;
  key.key.swap(cmd[1]);
  key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());
  // hashtable lookup
  HNode *node = hm_lookup(&g_data.db, &key.node, &entry_eq);
  if (!node) {
    return out_nil(out);
  }
  // copy the value
  Entry *ent = container_of(node, Entry, node);
  if (ent->type != T_STR) {
    return out_err(out, ERR_BAD_TYP, "not a string value");
  }
  return out_str(out, ent->str.data(), ent->str.size());
}

void do_set(std::vector<std::string> &cmd, Buffer &out) {
  // a dummy struct just for the lookup
  LookupKey key;
  key.key.swap(cmd[1]);
  key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());
  // hashtable lookup
  HNode *node = hm_lookup(&g_data.db, &key.node, &entry_eq);
  if (node) {
    // found, update the value
    Entry *ent = container_of(node, Entry, node);
    if (ent->type != T_STR) {
      return out_err(out, ERR_BAD_TYP, "a non-string value exists");
    }
    ent->str.swap(cmd[2]);
  } else {
    // not found, allocate & insert a new pair
    Entry *ent = entry_new(T_STR);
    ent->key.swap(key.key);
    ent->node.hcode = key.node.hcode;
    ent->str.swap(cmd[2]);
    hm_insert(&g_data.db, &ent->node);
  }
  return out_nil(out);
}

void do_del(std::vector<std::string> &cmd, Buffer &out) {
  // a dummy struct just for the lookup
  LookupKey key;
  key.key.swap(cmd[1]);
  key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());
  // hashtable delete
  HNode *node = hm_delete(&g_data.db, &key.node, &entry_eq);
  if (node) { // deallocate the pair
    entry_del(container_of(node, Entry, node));
  }
  return out_int(out, node ? 1 : 0);
}

static bool str2int(const std::string &s, int64_t &out) {
  char *endp = NULL;
  out = strtoll(s.c_str(), &endp, 10);
  return endp == s.c_str() + s.size();
}

// PEXPIRE key ttl_ms
void do_expire(std::vector<std::string> &cmd, Buffer &out) {
  int64_t ttl_ms = 0;
  if (!str2int(cmd[2], ttl_ms)) {
    return out_err(out, ERR_BAD_ARG, "expect int64");
  }

  LookupKey key;
  key.key.swap(cmd[1]);
  key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());

  HNode *node = hm_lookup(&g_data.db, &key.node, &entry_eq);
  if (node) {
    Entry *ent = container_of(node, Entry, node);
    entry_set_ttl(ent, ttl_ms);
  }
  return out_int(out, node ? 1 : 0);
}

// PTTL key
void do_ttl(std::vector<std::string> &cmd, Buffer &out) {
  LookupKey key;
  key.key.swap(cmd[1]);
  key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());

  HNode *node = hm_lookup(&g_data.db, &key.node, &entry_eq);
  if (!node) {
    return out_int(out, -2); // not found
  }

  Entry *ent = container_of(node, Entry, node);
  if (ent->heap_idx == (size_t)-1) {
    return out_int(out, -1); // no TTL
  }

  uint64_t expire_at = g_data.heap[ent->heap_idx].val;
  uint64_t now_ms = get_monotonic_msec();
  return out_int(out, expire_at > now_ms ? (expire_at - now_ms) : 0);
}

bool cb_keys(HNode *node, void *arg) {
  Buffer &out = *(Buffer *)arg;
  const std::string &key = container_of(node, Entry, node)->key;
  out_str(out, key.data(), key.size());
  return true;
}

void do_keys(std::vector<std::string> &, Buffer &out) {
  out_arr(out, (uint32_t)hm_size(&g_data.db));
  hm_foreach(&g_data.db, &cb_keys, (void *)&out);
}

static bool str2dbl(const std::string &s, double &out) {
  char *endp = NULL;
  out = strtod(s.c_str(), &endp);
  return endp == s.c_str() + s.size() && !std::isnan(out);
}

// zadd zset score name
void do_zadd(std::vector<std::string> &cmd, Buffer &out) {
  double score = 0;
  if (!str2dbl(cmd[2], score)) {
    return out_err(out, ERR_BAD_ARG, "expect float");
  }

  // look up or create the zset
  LookupKey key;
  key.key.swap(cmd[1]);
  key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());
  HNode *hnode = hm_lookup(&g_data.db, &key.node, &entry_eq);

  Entry *ent = NULL;
  if (!hnode) { // insert a new key
    ent = entry_new(T_ZSET);
    ent->key.swap(key.key);
    ent->node.hcode = key.node.hcode;
    hm_insert(&g_data.db, &ent->node);
  } else { // check the existing key
    ent = container_of(hnode, Entry, node);
    if (ent->type != T_ZSET) {
      return out_err(out, ERR_BAD_TYP, "expect zset");
    }
  }

  // add or update the tuple
  const std::string &name = cmd[3];
  bool added = zset_insert(&ent->zset, name.data(), name.size(), score);
  return out_int(out, (int64_t)added);
}

// zrem zset name
void do_zrem(std::vector<std::string> &cmd, Buffer &out) {
  ZSet *zset = expect_zset(cmd[1]);
  if (!zset) {
    return out_err(out, ERR_BAD_TYP, "expect zset");
  }

  const std::string &name = cmd[2];
  ZNode *znode = zset_lookup(zset, name.data(), name.size());
  if (znode) {
    zset_delete(zset, znode);
  }
  return out_int(out, znode ? 1 : 0);
}

// zscore zset name
void do_zscore(std::vector<std::string> &cmd, Buffer &out) {
  ZSet *zset = expect_zset(cmd[1]);
  if (!zset) {
    return out_err(out, ERR_BAD_TYP, "expect zset");
  }

  const std::string &name = cmd[2];
  ZNode *znode = zset_lookup(zset, name.data(), name.size());
  return znode ? out_dbl(out, znode->score) : out_nil(out);
}

// zquery zset score name offset limit
void do_zquery(std::vector<std::string> &cmd, Buffer &out) {
  // parse args
  double score = 0;
  if (!str2dbl(cmd[2], score)) {
    return out_err(out, ERR_BAD_ARG, "expect fp number");
  }
  const std::string &name = cmd[3];
  int64_t offset = 0, limit = 0;
  if (!str2int(cmd[4], offset) || !str2int(cmd[5], limit)) {
    return out_err(out, ERR_BAD_ARG, "expect int");
  }

  // get the zset
  ZSet *zset = expect_zset(cmd[1]);
  if (!zset) {
    return out_err(out, ERR_BAD_TYP, "expect zset");
  }

  // seek to the key
  if (limit <= 0) {
    return out_arr(out, 0);
  }
  ZNode *znode = zset_seekge(zset, score, name.data(), name.size());
  znode = znode_offset(znode, offset);

  // output
  size_t ctx = out_begin_arr(out);
  int64_t n = 0;
  while (znode && n < limit) {
    out_str(out, znode->name, znode->len);
    out_dbl(out, znode->score);
    znode = znode_offset(znode, +1);
    n += 2;
  }
  out_end_arr(out, ctx, (uint32_t)n);
}
