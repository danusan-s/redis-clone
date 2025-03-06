#include "protocol.h"
#include "commands.h"
#include <cassert>
#include <string.h>

// Helper functions for parsing
static bool read_u32(const uint8_t *&cur, const uint8_t *end, uint32_t &out) {
  if (cur + 4 > end) {
    return false;
  }
  memcpy(&out, cur, 4);
  cur += 4;
  return true;
}

static bool read_str(const uint8_t *&cur, const uint8_t *end, size_t n,
                     std::string &out) {
  if (cur + n > end) {
    return false;
  }
  out.assign(cur, cur + n);
  cur += n;
  return true;
}

// Parse request
// Format: +------+-----+------+-----+------+-----+-----+------+
//         | nstr | len | str1 | len | str2 | ... | len | strn |
//         +------+-----+------+-----+------+-----+-----+------+
int32_t parse_req(const uint8_t *data, size_t size,
                  std::vector<std::string> &out) {
  const uint8_t *end = data + size;
  uint32_t nstr = 0;
  if (!read_u32(data, end, nstr)) {
    return -1;
  }
  if (nstr > k_max_args) {
    return -1; // Safety limit
  }

  while (out.size() < nstr) {
    uint32_t len = 0;
    if (!read_u32(data, end, len)) {
      return -1;
    }
    out.push_back(std::string());
    if (!read_str(data, end, len, out.back())) {
      return -1;
    }
  }
  if (data != end) {
    return -1; // Trailing garbage
  }
  return 0;
}

// Output serialization functions
void out_nil(Buffer &out) { buf_append_u8(out, TAG_NIL); }

void out_str(Buffer &out, const char *s, size_t size) {
  buf_append_u8(out, TAG_STR);
  buf_append_u32(out, (uint32_t)size);
  buf_append(out, (const uint8_t *)s, size);
}

void out_int(Buffer &out, int64_t val) {
  buf_append_u8(out, TAG_INT);
  buf_append_i64(out, val);
}

void out_dbl(Buffer &out, double val) {
  buf_append_u8(out, TAG_DBL);
  buf_append_dbl(out, val);
}

void out_err(Buffer &out, uint32_t code, const std::string &msg) {
  buf_append_u8(out, TAG_ERR);
  buf_append_u32(out, code);
  buf_append_u32(out, (uint32_t)msg.size());
  buf_append(out, (const uint8_t *)msg.data(), msg.size());
}

void out_arr(Buffer &out, uint32_t n) {
  buf_append_u8(out, TAG_ARR);
  buf_append_u32(out, n);
}

size_t out_begin_arr(Buffer &out) {
  out.push_back(TAG_ARR);
  buf_append_u32(out, 0); // Filled by out_end_arr()
  return out.size() - 4;  // The `ctx` arg
}

void out_end_arr(Buffer &out, size_t ctx, uint32_t n) {
  assert(out[ctx - 1] == TAG_ARR);
  memcpy(&out[ctx], &n, 4);
}

// Response formatting
void response_begin(Buffer &out, size_t *header) {
  *header = out.size();   // Message header position
  buf_append_u32(out, 0); // Reserve space
}

size_t response_size(Buffer &out, size_t header) {
  return out.size() - header - 4;
}

void response_end(Buffer &out, size_t header) {
  size_t msg_size = response_size(out, header);
  if (msg_size > k_max_msg) {
    out.resize(header + 4);
    out_err(out, ERR_TOO_BIG, "response is too big.");
    msg_size = response_size(out, header);
  }
  // Message header
  uint32_t len = (uint32_t)msg_size;
  memcpy(&out[header], &len, 4);
}

// Process commands
void do_request(std::vector<std::string> &cmd, Buffer &out) {
  if (cmd.size() == 2 && cmd[0] == "get") {
    return do_get(cmd, out);
  } else if (cmd.size() == 3 && cmd[0] == "set") {
    return do_set(cmd, out);
  } else if (cmd.size() == 2 && cmd[0] == "del") {
    return do_del(cmd, out);
  } else if (cmd.size() == 3 && cmd[0] == "pexpire") {
    return do_expire(cmd, out);
  } else if (cmd.size() == 2 && cmd[0] == "pttl") {
    return do_ttl(cmd, out);
  } else if (cmd.size() == 1 && cmd[0] == "keys") {
    return do_keys(cmd, out);
  } else if (cmd.size() == 4 && cmd[0] == "zadd") {
    return do_zadd(cmd, out);
  } else if (cmd.size() == 3 && cmd[0] == "zrem") {
    return do_zrem(cmd, out);
  } else if (cmd.size() == 3 && cmd[0] == "zscore") {
    return do_zscore(cmd, out);
  } else if (cmd.size() == 6 && cmd[0] == "zquery") {
    return do_zquery(cmd, out);
  } else {
    return out_err(out, ERR_UNKNOWN, "unknown command.");
  }
}
