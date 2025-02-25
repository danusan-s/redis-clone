#include <cassert>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

const size_t k_max_msg = 4096;

static int32_t read_full(int fd, char *buf, size_t n) {
  while (n > 0) {
    ssize_t rv = read(fd, buf, n);
    if (rv <= 0) {
      return -1; // error, or unexpected EOF
    }
    assert((size_t)rv <= n);
    n -= (size_t)rv;
    buf += rv;
  }
  return 0;
}

static int32_t write_all(int fd, const char *buf, size_t n) {
  while (n > 0) {
    ssize_t rv = write(fd, buf, n);
    if (rv <= 0) {
      return -1; // error
    }
    assert((size_t)rv <= n);
    n -= (size_t)rv;
    buf += rv;
  }
  return 0;
}

static int32_t process_single_request(int connfd) {
  // 4 bytes header
  char rbuf[4 + k_max_msg];
  errno = 0;
  int32_t err = read_full(connfd, rbuf, 4);
  if (err) {
    std::cout << (errno == 0 ? "EOF" : "read() error") << std::endl;
    return err;
  }
  uint32_t len = 0;
  memcpy(&len, rbuf, 4); // assume little endian
  if (len > k_max_msg) {
    std::cout << "message too long" << std::endl;
    return -1;
  }
  // request body
  err = read_full(connfd, &rbuf[4], len);
  if (err) {
    std::cout << "read() error" << std::endl;
    return err;
  }

  printf("client says: %.*s\n", len, &rbuf[4]);

  // reply using the same protocol
  const char reply[] = "world";
  char wbuf[4 + sizeof(reply)];
  len = (uint32_t)strlen(reply);
  memcpy(wbuf, &len, 4);
  memcpy(&wbuf[4], reply, len);
  return write_all(connfd, wbuf, 4 + len);
}

int main(int argc, char *argv[]) {
  // AF_INET for IPv4 and SOCK_STREAM for TCP
  int fd = socket(AF_INET, SOCK_STREAM, 0);

  // Set REUSEADDR option so server can bind to same addr:port after restart
  int val = 1;
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

  // Set up the address and port to bind to
  // htons -> host to network short, convert system 16-bit value to big endian
  // htonl -> host to network long, convert system 32-bit value to big endian
  struct sockaddr_in addr = {};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(1234);     // port
  addr.sin_addr.s_addr = htonl(0); // wildcard IP 0.0.0.0

  // Bind the socket to the address and port
  int rv = bind(fd, (const struct sockaddr *)&addr, sizeof(addr));
  if (rv) {
    std::cout << "Error: " << strerror(errno) << std::endl;
  }

  // Listen for incoming connections
  // SOMAXCONN is the queue size for pending connections (SOMAXCONN = 4096)
  rv = listen(fd, SOMAXCONN);
  if (rv) {
    std::cout << "Error: " << strerror(errno) << std::endl;
  }

  while (true) {
    // Accept a connection from a client
    struct sockaddr_in client_addr = {};
    socklen_t addrlen = sizeof(client_addr);
    int client_fd = accept(fd, (struct sockaddr *)&client_addr, &addrlen);
    if (client_fd == -1) {
      std::cout << "Error: " << strerror(errno) << std::endl;
    }

    // Process all requests from the client one by one
    while (true) {
      int32_t err = process_single_request(client_fd);
      if (err) {
        break;
      }
    }

    // Close the client connection
    close(client_fd);
  }
}
