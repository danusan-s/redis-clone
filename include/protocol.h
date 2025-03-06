#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "buffer.h"
#include <string>

// Constants
const size_t k_max_args = 200 * 1000;
const size_t k_max_msg = 32 << 20;

// Error codes for TAG_ERR
enum {
  ERR_UNKNOWN = 1, // Unknown command
  ERR_TOO_BIG = 2, // Response too big
  ERR_BAD_TYP = 3, // Unexpected value type
  ERR_BAD_ARG = 4, // Bad arguments
};

// Data types of serialized data
enum {
  TAG_NIL = 0, // Nil
  TAG_ERR = 1, // Error code + msg
  TAG_STR = 2, // String
  TAG_INT = 3, // Int64
  TAG_DBL = 4, // Double
  TAG_ARR = 5, // Array
};

// Protocol parsing
int32_t parse_req(const uint8_t *data, size_t size,
                  std::vector<std::string> &out);

// Protocol serialization
void out_nil(Buffer &out);
void out_str(Buffer &out, const char *s, size_t size);
void out_int(Buffer &out, int64_t val);
void out_dbl(Buffer &out, double val);
void out_err(Buffer &out, uint32_t code, const std::string &msg);
void out_arr(Buffer &out, uint32_t n);
size_t out_begin_arr(Buffer &out);
void out_end_arr(Buffer &out, size_t ctx, uint32_t n);

// Response formatting
void response_begin(Buffer &out, size_t *header);
size_t response_size(Buffer &out, size_t header);
void response_end(Buffer &out, size_t header);

// Command processing
void do_request(std::vector<std::string> &cmd, Buffer &out);

#endif // PROTOCOL_H
