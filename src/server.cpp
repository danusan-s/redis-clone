#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

static void process_client(int fd) {
  // We can replace read/write with recv/send if we want to specify more options

  char rbuf[64];
  ssize_t n = read(fd, rbuf, sizeof(rbuf) - 1);
  if (n < 0) {
    std::cout << "Error: " << strerror(errno) << std::endl;
    return;
  }
  if (n == 0) {
    std::cout << "Client closed connection" << std::endl;
    return;
  }
  rbuf[n] = '\0';
  printf("Received %ld bytes: %s\n", n, rbuf);

  char wbuf[] = "Hello, client!";
  write(fd, wbuf, strlen(wbuf));
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
    // Accept a connection
    struct sockaddr_in client_addr = {};
    socklen_t addrlen = sizeof(client_addr);
    int client_fd = accept(fd, (struct sockaddr *)&client_addr, &addrlen);
    if (client_fd == -1) {
      std::cout << "Error: " << strerror(errno) << std::endl;
    }

    // Do something with the client connection
    process_client(client_fd);

    // Close the client connection
    close(client_fd);
  }
}
