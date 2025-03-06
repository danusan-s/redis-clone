#ifndef CONNECTION_H
#define CONNECTION_H

#include "global_state.h"
#include <cstdint>

// Connection operations
int32_t handle_accept(int fd);
void conn_destroy(Conn *conn);
void handle_read(Conn *conn);
void handle_write(Conn *conn);
bool try_one_request(Conn *conn);

#endif // CONNECTION_H
