#define _POSIX_C_SOURCE 200809L
#undef NDEBUG

#include <arpa/inet.h>
#include <assert.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "common.h"
#include "metadata/core.h"
#include "metadata/pc_queue.h"

shairport_cfg config;

static int stub_send_metadata_to_queue_rc;
static unsigned stub_send_metadata_to_queue_calls;
static pc_queue *stub_last_queue;
static uint32_t stub_last_type;
static uint32_t stub_last_code;
static const char *stub_last_data;
static uint32_t stub_last_length;
static rtsp_message *stub_last_carrier;
static int stub_last_block;

static int test_socket_calls;
static int test_shutdown_calls;
static int test_close_calls;
static int test_setsockopt_calls;
static int test_sendto_calls;
static int test_sendto_fail_with_errno;
static int test_last_shutdown_fd;
static int test_last_close_fd;

struct packet_capture {
  size_t len;
  unsigned char data[256];
};

static struct packet_capture test_packets[16];

static int test_socket(__attribute__((unused)) int domain, __attribute__((unused)) int type,
                       __attribute__((unused)) int protocol) {
  test_socket_calls++;
  return 7;
}

static int test_setsockopt(__attribute__((unused)) int socket, __attribute__((unused)) int level,
                           __attribute__((unused)) int option_name,
                           __attribute__((unused)) const void *option_value,
                           __attribute__((unused)) socklen_t option_len) {
  test_setsockopt_calls++;
  return 0;
}

static in_addr_t test_inet_addr(__attribute__((unused)) const char *cp) {
  return htonl(0x7f000001U);
}

static int test_shutdown(int fd, __attribute__((unused)) int how) {
  test_shutdown_calls++;
  test_last_shutdown_fd = fd;
  return 0;
}

static int test_close(int fd) {
  test_close_calls++;
  test_last_close_fd = fd;
  return 0;
}

static ssize_t test_sendto(__attribute__((unused)) int sockfd, const void *buf, size_t len,
                           __attribute__((unused)) int flags,
                           __attribute__((unused)) const struct sockaddr *dest_addr,
                           __attribute__((unused)) socklen_t addrlen) {
  if (test_sendto_calls < (int)(sizeof(test_packets) / sizeof(test_packets[0]))) {
    size_t copy_len = len;
    if (copy_len > sizeof(test_packets[0].data))
      copy_len = sizeof(test_packets[0].data);
    test_packets[test_sendto_calls].len = len;
    memcpy(test_packets[test_sendto_calls].data, buf, copy_len);
  }
  test_sendto_calls++;
  if (test_sendto_fail_with_errno != 0) {
    errno = test_sendto_fail_with_errno;
    return -1;
  }
  return (ssize_t)len;
}

#define socket test_socket
#define setsockopt test_setsockopt
#define inet_addr test_inet_addr
#define shutdown test_shutdown
#define close test_close
#define sendto test_sendto
#include "../metadata/multicast.c"
#undef socket
#undef setsockopt
#undef inet_addr
#undef shutdown
#undef close
#undef sendto

static void reset_state(void) {
  stub_send_metadata_to_queue_rc = 0;
  stub_send_metadata_to_queue_calls = 0;
  stub_last_queue = NULL;
  stub_last_type = 0;
  stub_last_code = 0;
  stub_last_data = NULL;
  stub_last_length = 0;
  stub_last_carrier = NULL;
  stub_last_block = -1;
  test_socket_calls = 0;
  test_shutdown_calls = 0;
  test_close_calls = 0;
  test_setsockopt_calls = 0;
  test_sendto_calls = 0;
  test_sendto_fail_with_errno = 0;
  test_last_shutdown_fd = -1;
  test_last_close_fd = -1;
  memset(test_packets, 0, sizeof(test_packets));

  if (metadata_sockmsg != NULL) {
    free(metadata_sockmsg);
    metadata_sockmsg = NULL;
  }
  metadata_sock = -1;
  memset(&metadata_sockaddr, 0, sizeof(metadata_sockaddr));

  memset(&config, 0, sizeof(config));
  config.metadata_enabled = 1;
  config.metadata_sockaddr = "127.0.0.1";
  config.metadata_sockport = 9999;
  config.metadata_sockmsglength = 64;
}

static void test_cleanup_releases_even_when_disabled(__attribute__((unused)) void **state) {
  reset_state();
  metadata_sock = 11;
  metadata_sockmsg = malloc(32);
  assert(metadata_sockmsg != NULL);

  config.metadata_enabled = 0;
  metadata_delete_multicast_socket();
  assert(test_shutdown_calls == 1);
  assert(test_close_calls == 1);
  assert(test_last_shutdown_fd == 11);
  assert(test_last_close_fd == 11);
  assert(metadata_sock == -1);
  assert(metadata_sockmsg == NULL);
}

static void test_unchunked_packet_format(__attribute__((unused)) void **state) {
  const char payload[] = "abc";
  uint32_t type = METADATA_FOURCC('c', 'o', 'r', 'e');
  uint32_t code = METADATA_FOURCC('m', 'i', 'n', 'm');
  uint32_t v;

  reset_state();
  metadata_sock = 9;
  metadata_sockmsg = malloc(config.metadata_sockmsglength);
  assert(metadata_sockmsg != NULL);

  metadata_multicast_process(type, code, (char *)payload, (uint32_t)strlen(payload));

  assert(test_sendto_calls == 1);
  assert(test_packets[0].len == strlen(payload) + 8);
  memcpy(&v, test_packets[0].data, 4);
  assert(v == htonl(type));
  memcpy(&v, test_packets[0].data + 4, 4);
  assert(v == htonl(code));
  assert(memcmp(test_packets[0].data + 8, payload, strlen(payload)) == 0);
}

static void test_chunked_packet_format(__attribute__((unused)) void **state) {
  const char payload[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  uint32_t type = METADATA_FOURCC('c', 'o', 'r', 'e');
  uint32_t code = METADATA_FOURCC('a', 's', 't', 'm');
  uint32_t v;

  reset_state();
  config.metadata_sockmsglength = 32; // 8-byte data per packet after 24-byte header
  metadata_sock = 10;
  metadata_sockmsg = malloc(config.metadata_sockmsglength);
  assert(metadata_sockmsg != NULL);

  metadata_multicast_process(type, code, (char *)payload, (uint32_t)strlen(payload));

  assert(test_sendto_calls == 4);
  assert(test_packets[0].len == 32);
  assert(test_packets[1].len == 32);
  assert(test_packets[2].len == 32);
  assert(test_packets[3].len == 26);

  for (int i = 0; i < 4; i++) {
    assert(memcmp(test_packets[i].data, "ssncchnk", 8) == 0);
    memcpy(&v, test_packets[i].data + 8, 4);
    assert(v == htonl((uint32_t)i));
    memcpy(&v, test_packets[i].data + 12, 4);
    assert(v == htonl(4));
    memcpy(&v, test_packets[i].data + 16, 4);
    assert(v == htonl(type));
    memcpy(&v, test_packets[i].data + 20, 4);
    assert(v == htonl(code));
  }

  assert(memcmp(test_packets[0].data + 24, "ABCDEFGH", 8) == 0);
  assert(memcmp(test_packets[1].data + 24, "IJKLMNOP", 8) == 0);
  assert(memcmp(test_packets[2].data + 24, "QRSTUVWX", 8) == 0);
  assert(memcmp(test_packets[3].data + 24, "YZ", 2) == 0);
}

static void test_invalid_sockmsglength_skips_send(__attribute__((unused)) void **state) {
  reset_state();
  config.metadata_sockmsglength = 24;
  metadata_sock = 12;
  metadata_sockmsg = malloc(24);
  assert(metadata_sockmsg != NULL);

  metadata_multicast_process(METADATA_FOURCC('s', 's', 'n', 'c'),
                             METADATA_FOURCC('a', 'r', 's', 't'), "x", 1);
  assert(test_sendto_calls == 0);
}

static void test_send_metadata_to_multicast_queue_forwards(__attribute__((unused)) void **state) {
  rtsp_message carrier = {0};

  reset_state();
  stub_send_metadata_to_queue_rc = EWOULDBLOCK;
  assert(send_metadata_to_multicast_queue(METADATA_FOURCC('s', 's', 'n', 'c'),
                                          METADATA_FOURCC('a', 'r', 's', 't'), "x", 1, &carrier,
                                          1) == EWOULDBLOCK);
  assert(stub_send_metadata_to_queue_calls == 1);
  assert(stub_last_queue == &metadata_multicast_queue);
  assert(stub_last_type == METADATA_FOURCC('s', 's', 'n', 'c'));
  assert(stub_last_code == METADATA_FOURCC('a', 'r', 's', 't'));
  assert(stub_last_data != NULL && stub_last_data[0] == 'x');
  assert(stub_last_length == 1);
  assert(stub_last_carrier == &carrier);
  assert(stub_last_block == 1);
}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test_cleanup_releases_even_when_disabled),
      cmocka_unit_test(test_unchunked_packet_format),
      cmocka_unit_test(test_chunked_packet_format),
      cmocka_unit_test(test_invalid_sockmsglength_skips_send),
      cmocka_unit_test(test_send_metadata_to_multicast_queue_forwards),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}

int send_metadata_to_queue(pc_queue *queue, const uint32_t type, const uint32_t code,
                           const char *data, const uint32_t length, rtsp_message *carrier,
                           int block) {
  stub_send_metadata_to_queue_calls++;
  stub_last_queue = queue;
  stub_last_type = type;
  stub_last_code = code;
  stub_last_data = data;
  stub_last_length = length;
  stub_last_carrier = carrier;
  stub_last_block = block;
  return stub_send_metadata_to_queue_rc;
}

void metadata_pack_cleanup_function(__attribute__((unused)) void *arg) {}
void metadata_queue_delete(__attribute__((unused)) pc_queue *queue) {}

int named_pthread_create(__attribute__((unused)) pthread_t *restrict thread,
                         __attribute__((unused)) const pthread_attr_t *restrict attr,
                         __attribute__((unused)) void *(*start_routine)(void *),
                         __attribute__((unused)) void *restrict arg,
                         __attribute__((unused)) const char *format, ...) {
  return 0;
}

void pc_queue_init(__attribute__((unused)) pc_queue *the_queue, __attribute__((unused)) char *items,
                   __attribute__((unused)) size_t item_size,
                   __attribute__((unused)) uint32_t number_of_items,
                   __attribute__((unused)) const char *name) {}

void pc_queue_delete(__attribute__((unused)) pc_queue *the_queue) {}

int pc_queue_add_item(__attribute__((unused)) pc_queue *the_queue,
                      __attribute__((unused)) const void *the_stuff,
                      __attribute__((unused)) int block) {
  return 0;
}

int pc_queue_get_item(__attribute__((unused)) pc_queue *the_queue,
                      __attribute__((unused)) void *the_stuff) {
  return 0;
}

void _debug(__attribute__((unused)) const char *filename, __attribute__((unused)) const int linenumber,
            __attribute__((unused)) int level, __attribute__((unused)) const char *format, ...) {}

void _die(__attribute__((unused)) const char *filename, __attribute__((unused)) const int linenumber,
          __attribute__((unused)) const char *format, ...) {
  abort();
}
