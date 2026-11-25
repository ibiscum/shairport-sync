#include <errno.h>
#include <netdb.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/socket.h>

#include "test_ptp_utilities_harness.h"
#include "utilities/network_utilities.h"

shairport_cfg config;

int ptp_test_getaddrinfo_calls = 0;
int ptp_test_socket_calls = 0;
int ptp_test_sendto_calls = 0;

void reset_ptp_utilities_stubs(void) {
  ptp_test_getaddrinfo_calls = 0;
  ptp_test_socket_calls = 0;
  ptp_test_sendto_calls = 0;
  config.nqptp_shared_memory_interface_name = NULL;
}

int getaddrinfo(const char *node, const char *service, const struct addrinfo *hints,
                struct addrinfo **res) {
  (void)node;
  (void)service;
  (void)hints;
  (void)res;
  ptp_test_getaddrinfo_calls++;
  return EAI_FAIL;
}

void freeaddrinfo(struct addrinfo *res) { (void)res; }

int socket(int domain, int type, int protocol) {
  (void)domain;
  (void)type;
  (void)protocol;
  ptp_test_socket_calls++;
  errno = EIO;
  return -1;
}

ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
               const struct sockaddr *dest_addr, socklen_t addrlen) {
  (void)sockfd;
  (void)buf;
  (void)len;
  (void)flags;
  (void)dest_addr;
  (void)addrlen;
  ptp_test_sendto_calls++;
  errno = EIO;
  return -1;
}

int _safe_socket_close(const char *filename, const int linenumber, int *sockfd) {
  (void)filename;
  (void)linenumber;
  if (sockfd)
    *sockfd = -1;
  return 0;
}

void _debug(const char *filename, const int linenumber, int level, const char *format, ...) {
  (void)filename;
  (void)linenumber;
  (void)level;
  (void)format;
}

void _inform(const char *filename, const int linenumber, const char *format, ...) {
  (void)filename;
  (void)linenumber;
  (void)format;
}

void _warn(const char *filename, const int linenumber, const char *format, ...) {
  (void)filename;
  (void)linenumber;
  (void)format;
}

void _die(const char *filename, const int linenumber, const char *format, ...) {
  (void)filename;
  (void)linenumber;
  (void)format;
  abort();
}
