#ifndef BUFFER_H
#define BUFFER_H

#include <stdint.h>
#include <vector>

// Buffer type for data handling
typedef std::vector<uint8_t> Buffer;

// Buffer operations
void buf_append(Buffer &buf, const uint8_t *data, size_t len);
void buf_consume(Buffer &buf, size_t n);
void buf_append_u8(Buffer &buf, uint8_t data);
void buf_append_u32(Buffer &buf, uint32_t data);
void buf_append_i64(Buffer &buf, int64_t data);
void buf_append_dbl(Buffer &buf, double data);

#endif // BUFFER_H
