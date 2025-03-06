// stdlib
#include <assert.h>
#include <ctime>
#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// system
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/ip.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
// C++
#include <vector>
// custom
#include "connection_manager.h"
#include "dlist.h"
#include "global_state.h"
#include "shared.h"
#include "thread_pool.h"
#include "timer.h"

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

int main() {
  dlist_init(&g_data.idle_list);
  thread_pool_init(&g_data.thread_pool, 4);

  // the listening socket
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    die("socket()");
  }
  int val = 1;
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

  // bind
  struct sockaddr_in addr = {};
  addr.sin_family = AF_INET;
  addr.sin_port = ntohs(1234);
  addr.sin_addr.s_addr = ntohl(0); // wildcard address 0.0.0.0
  int rv = bind(fd, (const sockaddr *)&addr, sizeof(addr));
  if (rv) {
    die("bind()");
  }

  // set the listen fd to nonblocking mode
  fd_set_nb(fd);

  // listen
  rv = listen(fd, SOMAXCONN);
  if (rv) {
    die("listen()");
  }

  // The event loop
  std::vector<struct pollfd> poll_args;
  while (true) {
    // prepare the arguments of the poll()
    poll_args.clear();
    // put the listening sockets in the first position
    struct pollfd pfd = {fd, POLLIN, 0};
    poll_args.push_back(pfd);
    // the rest are connection sockets
    for (Conn *conn : g_data.fd2conn) {
      if (!conn) {
        continue;
      }
      // always poll() for error
      struct pollfd pfd = {conn->fd, POLLERR, 0};
      // poll() flags from the application's intent
      if (conn->want_read) {
        pfd.events |= POLLIN;
      }
      if (conn->want_write) {
        pfd.events |= POLLOUT;
      }
      poll_args.push_back(pfd);
    }

    // wait for readiness
    int32_t timeout_ms = next_timer_ms();
    int rv = poll(poll_args.data(), (nfds_t)poll_args.size(), timeout_ms);
    if (rv < 0 && errno == EINTR) {
      continue; // not an error
    }
    if (rv < 0) {
      die("poll");
    }

    // handle the listening socket
    if (poll_args[0].revents) {
      handle_accept(fd);
    }

    // handle connection sockets
    for (size_t i = 1; i < poll_args.size(); ++i) { // note: skip the 1st
      uint32_t ready = poll_args[i].revents;
      if (ready == 0) {
        continue;
      }
      Conn *conn = g_data.fd2conn[poll_args[i].fd];

      // update the idle timer by moving conn to the end of the list
      conn->last_active_ms = get_monotonic_msec();
      dlist_detach(&conn->idle_node);
      dlist_insert_before(&g_data.idle_list, &conn->idle_node);

      // handle IO
      if (ready & POLLIN) {
        assert(conn->want_read);
        handle_read(conn); // application logic
      }
      if (ready & POLLOUT) {
        assert(conn->want_write);
        handle_write(conn); // application logic
      }

      // close the socket from socket error or application logic
      if ((ready & POLLERR) || conn->want_close) {
        conn_destroy(conn);
      }
    } // for each connection sockets

    // handle timers
    process_timers();
  } // the event loop
  return 0;
}
