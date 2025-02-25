#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    std::cout << "Error: " << strerror(errno) << std::endl;
    return 1;
  }

  struct sockaddr_in addr = {};
  addr.sin_family = AF_INET;
  addr.sin_port = ntohs(1234);
  addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK);

  int rv = connect(fd, (const struct sockaddr *)&addr, sizeof(addr));
  if (rv < 0) {
    std::cout << "Error: " << strerror(errno) << std::endl;
    return 1;
  }

  char wbuf[] = "Hello, server!";
  write(fd, wbuf, strlen(wbuf));

  char rbuf[64];
  ssize_t n = read(fd, rbuf, sizeof(rbuf) - 1);
  if (n < 0) {
    std::cout << "Error: " << strerror(errno) << std::endl;
    return 1;
  }

  std::cout << "Received " << n << " bytes: " << rbuf << std::endl;
  close(fd);
}
