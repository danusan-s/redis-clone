#include "connection_manager.h"
#include "protocol.h"
#include "shared.h"
#include "timer.h"
#include <arpa/inet.h>
#include <cassert>
#include <fcntl.h>
#include <netinet/ip.h>
#include <poll.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

// set the fd to nonblocking mode
static void fd_set_nb(int fd) {
  errno = 0;
  int flags = fcntl(fd, F_GETFL, 0);
  if (errno) {
    die("fcntl error");
    return;
  }

  flags |= O_NONBLOCK;

  errno = 0;
  (void)fcntl(fd, F_SETFL, flags);
  if (errno) {
    die("fcntl error");
  }
}

// Accept a new connection
int32_t handle_accept(int fd) {
  // Accept
  struct sockaddr_in client_addr = {};
  socklen_t socklen = sizeof(client_addr);
  int connfd = accept(fd, (struct sockaddr *)&client_addr, &socklen);
  if (connfd < 0) {
    msg_errno("accept() error");
    return -1;
  }

  uint32_t ip = client_addr.sin_addr.s_addr;
  fprintf(stderr, "new client from %u.%u.%u.%u:%u\n", ip & 255, (ip >> 8) & 255,
          (ip >> 16) & 255, ip >> 24, ntohs(client_addr.sin_port));

  // Set the new connection fd to nonblocking mode
  fd_set_nb(connfd);

  // Create a `struct Conn`
  Conn *conn = new Conn();
  conn->fd = connfd;
  conn->want_read = true;
  conn->last_active_ms = get_monotonic_msec();
  dlist_insert_before(&g_data.idle_list, &conn->idle_node);

  // Put it into the map
  if (g_data.fd2conn.size() <= (size_t)conn->fd) {
    g_data.fd2conn.resize(conn->fd + 1);
  }
  assert(!g_data.fd2conn[conn->fd]);
  g_data.fd2conn[conn->fd] = conn;

  return 0;
}

// Destroy a connection
void conn_destroy(Conn *conn) {
  (void)close(conn->fd);
  g_data.fd2conn[conn->fd] = NULL;
  dlist_detach(&conn->idle_node);
  delete conn;
}

// Handle read events
void handle_read(Conn *conn) {
  // Read some data
  uint8_t buf[64 * 1024];
  ssize_t rv = read(conn->fd, buf, sizeof(buf));

  if (rv < 0 && errno == EAGAIN) {
    return; // Actually not ready
  }

  if (rv < 0) {
    msg_errno("read() error");
    conn->want_close = true;
    return; // Want close
  }

  // Handle EOF
  if (rv == 0) {
    if (conn->incoming.size() == 0) {
      msg("client closed");
    } else {
      msg("unexpected EOF");
    }
    conn->want_close = true;
    return; // Want close
  }

  // Append to the incoming buffer
  buf_append(conn->incoming, buf, (size_t)rv);

  // Keep processing requests until there is not enough data
  while (try_one_request(conn))
    ;

  // Update the readiness intention if a response is ready
  if (conn->outgoing.size() > 0) {
    conn->want_read = false;
    conn->want_write = true;
    // The socket is likely ready to write in a request-response protocol,
    // we can write the response without waiting for next poll() cycle.
    // But we don't do the same in the read callback because we want to
    // terminate and go back to the event loop to handle other connections
    return handle_write(conn);
  } // Else: want read
}

// Handle write events
void handle_write(Conn *conn) {
  assert(conn->outgoing.size() > 0);

  // Write some data (might not write all)
  ssize_t rv = write(conn->fd, &conn->outgoing[0], conn->outgoing.size());

  if (rv < 0 && errno == EAGAIN) {
    return; // Actually not ready
  }

  if (rv < 0) {
    msg_errno("write() error");
    conn->want_close = true; // Error handling
    return;
  }

  // Remove written data from `outgoing`
  buf_consume(conn->outgoing, (size_t)rv);

  // Update the readiness intention if all data written
  if (conn->outgoing.size() == 0) {
    conn->want_read = true;
    conn->want_write = false;
  } // Else: want write
}

// Process one request if there is enough data
bool try_one_request(Conn *conn) {
  // Try to parse the protocol: message header
  if (conn->incoming.size() < 4) {
    return false; // Want read
  }

  uint32_t len = 0;
  memcpy(&len, conn->incoming.data(), 4);
  if (len > k_max_msg) {
    msg("too long");
    conn->want_close = true;
    return false; // Want close
  }

  // Message body
  if (4 + len > conn->incoming.size()) {
    return false; // Want read
  }

  const uint8_t *request = &conn->incoming[4];

  // Got one request, do some application logic
  std::vector<std::string> cmd;
  if (parse_req(request, len, cmd) < 0) {
    msg("bad request");
    conn->want_close = true;
    return false; // Want close
  }

  size_t header_pos = 0;
  response_begin(conn->outgoing, &header_pos);
  do_request(cmd, conn->outgoing);
  response_end(conn->outgoing, header_pos);

  // Application logic done! Remove the request message.
  buf_consume(conn->incoming, 4 + len);

  return true; // Success
}
