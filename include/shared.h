#pragma once

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <stddef.h>
#include <stdint.h>

// intrusive data structure
#define container_of(ptr, type, member)                                        \
  ({                                                                           \
    const typeof(((type *)0)->member) *__mptr = (ptr);                         \
    (type *)((char *)__mptr - offsetof(type, member));                         \
  })

// FNV hash
inline uint64_t str_hash(const uint8_t *data, size_t len) {
  uint32_t h = 0x811C9DC5;
  for (size_t i = 0; i < len; i++) {
    h = (h + data[i]) * 0x01000193;
  }
  return h;
}

// Logging function
inline void msg(const char *msg) { fprintf(stderr, "%s\n", msg); }

// Logging function with errno
inline void msg_errno(const char *msg) {
  fprintf(stderr, "[errno:%d] %s\n", errno, msg);
}

// Logging function with errno and abort after
inline void die(const char *msg) {
  fprintf(stderr, "[%d] %s\n", errno, msg);
  abort();
}
