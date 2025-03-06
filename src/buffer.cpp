#include "buffer.h"

// Append data to the buffer
void buf_append(Buffer &buf, const uint8_t *data, size_t len) {
  buf.insert(buf.end(), data, data + len);
}

// Remove data from the front of the buffer
void buf_consume(Buffer &buf, size_t n) {
  buf.erase(buf.begin(), buf.begin() + n);
}

// Append a uint8_t to the buffer
void buf_append_u8(Buffer &buf, uint8_t data) { buf.push_back(data); }

// Append a uint32_t to the buffer
void buf_append_u32(Buffer &buf, uint32_t data) {
  buf_append(buf, (const uint8_t *)&data, 4);
}

// Append an int64_t to the buffer
void buf_append_i64(Buffer &buf, int64_t data) {
  buf_append(buf, (const uint8_t *)&data, 8);
}

// Append a double to the buffer
void buf_append_dbl(Buffer &buf, double data) {
  buf_append(buf, (const uint8_t *)&data, 8);
}
