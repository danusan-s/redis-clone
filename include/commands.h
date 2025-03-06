#ifndef COMMANDS_H
#define COMMANDS_H

#include "buffer.h"
#include <string>

// Command handlers
void do_get(std::vector<std::string> &cmd, Buffer &out);
void do_set(std::vector<std::string> &cmd, Buffer &out);
void do_del(std::vector<std::string> &cmd, Buffer &out);
void do_expire(std::vector<std::string> &cmd, Buffer &out);
void do_ttl(std::vector<std::string> &cmd, Buffer &out);
void do_keys(std::vector<std::string> &cmd, Buffer &out);
void do_zadd(std::vector<std::string> &cmd, Buffer &out);
void do_zrem(std::vector<std::string> &cmd, Buffer &out);
void do_zscore(std::vector<std::string> &cmd, Buffer &out);
void do_zquery(std::vector<std::string> &cmd, Buffer &out);

#endif // COMMANDS_H
