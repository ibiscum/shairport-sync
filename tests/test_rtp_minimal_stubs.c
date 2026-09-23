#include <arpa/inet.h>
#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include <sodium.h>

#include "common.h"
#include "player.h"
#include "ptp-utilities.h"
#include "utilities/debug.h"

shairport_cfg config;

static uint64_t mock_absolute_time_ns = 0;
static uint16_t mock_bind_port_base = 6000;
static uint16_t mock_bind_port_next = 6000;
static int mock_sendto_should_fail = 0;

static int mock_ptp_response = clock_no_anchor_info;
static uint64_t mock_ptp_clock_id = 0;
static uint64_t mock_ptp_time_of_sample = 0;
static uint64_t mock_ptp_raw_offset = 0;
static uint64_t mock_ptp_mastership_start_time = 0;

int sendto_call_count = 0;
int player_put_packet_call_count = 0;
seq_t player_put_packet_last_seq = 0;
uint32_t player_put_packet_last_timestamp = 0;
size_t player_put_packet_last_len = 0;

void reset_rtp_test_stubs(void) {
  mock_absolute_time_ns = 0;
  mock_bind_port_base = 6000;
  mock_bind_port_next = mock_bind_port_base;
  mock_sendto_should_fail = 0;
  mock_ptp_response = clock_no_anchor_info;
  mock_ptp_clock_id = 0;
  mock_ptp_time_of_sample = 0;
  mock_ptp_raw_offset = 0;
  mock_ptp_mastership_start_time = 0;
  sendto_call_count = 0;
  player_put_packet_call_count = 0;
  player_put_packet_last_seq = 0;
  player_put_packet_last_timestamp = 0;
  player_put_packet_last_len = 0;
  memset(&config, 0, sizeof(config));
}

void set_mock_absolute_time_ns(uint64_t t) { mock_absolute_time_ns = t; }

void set_mock_bind_port_base(uint16_t p) {
  mock_bind_port_base = p;
  mock_bind_port_next = p;
}

void set_mock_sendto_response(int should_fail) {
  mock_sendto_should_fail = should_fail;
}

void set_mock_ptp_response(int response, uint64_t clock_id, uint64_t time_of_sample,
                           uint64_t raw_offset, uint64_t mastership_start_time) {
  mock_ptp_response = response;
  mock_ptp_clock_id = clock_id;
  mock_ptp_time_of_sample = time_of_sample;
  mock_ptp_raw_offset = raw_offset;
  mock_ptp_mastership_start_time = mastership_start_time;
}

void mutex_unlock(void *arg) { pthread_mutex_unlock((pthread_mutex_t *)arg); }

int named_pthread_create(pthread_t *restrict thread,
                         const pthread_attr_t *restrict attr,
                         void *(*start_routine)(void *), void *restrict arg,
                         const char *format, ...) {
  (void)thread;
  (void)attr;
  (void)start_routine;
  (void)arg;
  (void)format;
  return 0;
}

uint64_t get_absolute_time_in_ns(void) { return mock_absolute_time_ns; }

uint32_t nctohl(const uint8_t *p) {
  uint32_t t;
  memcpy(&t, p, sizeof(t));
  return ntohl(t);
}

uint16_t nctohs(const uint8_t *p) {
  uint16_t t;
  memcpy(&t, p, sizeof(t));
  return ntohs(t);
}

uint64_t nctoh64(const uint8_t *p) {
  uint64_t hi = nctohl(p);
  uint64_t lo = nctohl(p + 4);
  return (hi << 32) | lo;
}

uint16_t bind_UDP_port(int ip_family, const char *self_ip_address,
                       uint32_t scope_id, int *sock) {
  (void)ip_family;
  (void)self_ip_address;
  (void)scope_id;
  if (sock)
    *sock = mock_bind_port_next;
  return mock_bind_port_next++;
}

char *debug_malloc_hex_cstring(void *packet, size_t nread) {
  (void)packet;
  (void)nread;
  char *s = malloc(1);
  if (s)
    s[0] = '\0';
  return s;
}

int _safe_socket_close(const char *filename, const int linenumber, int *sockfd) {
  (void)filename;
  (void)linenumber;
  if (sockfd)
    *sockfd = -1;
  return 0;
}

uint32_t player_put_packet(uint32_t ssrc, seq_t seqno, uint32_t actual_timestamp,
                           uint8_t *data, size_t len, int mute,
                           int32_t timestamp_gap, rtsp_conn_info *conn) {
  (void)ssrc;
  (void)seqno;
  (void)actual_timestamp;
  (void)data;
  (void)len;
  (void)mute;
  (void)timestamp_gap;
  (void)conn;
  player_put_packet_call_count++;
  player_put_packet_last_seq = seqno;
  player_put_packet_last_timestamp = actual_timestamp;
  player_put_packet_last_len = len;
  return 0;
}

int ptp_get_clock_info(uint64_t *actual_clock_id, uint64_t *time_of_sample,
                       uint64_t *raw_offset, uint64_t *mastership_start_time) {
  if (actual_clock_id)
    *actual_clock_id = mock_ptp_clock_id;
  if (time_of_sample)
    *time_of_sample = mock_ptp_time_of_sample;
  if (raw_offset)
    *raw_offset = mock_ptp_raw_offset;
  if (mastership_start_time)
    *mastership_start_time = mock_ptp_mastership_start_time;
  return mock_ptp_response;
}

int crypto_aead_chacha20poly1305_ietf_decrypt(unsigned char *m,
                                               unsigned long long *mlen_p,
                                               unsigned char *nsec,
                                               const unsigned char *c,
                                               unsigned long long clen,
                                               const unsigned char *ad,
                                               unsigned long long adlen,
                                               const unsigned char *npub,
                                               const unsigned char *k) {
  (void)nsec;
  (void)ad;
  (void)adlen;
  (void)npub;
  (void)k;
  if (clen < crypto_aead_chacha20poly1305_ietf_ABYTES)
    return -1;
  if (mlen_p)
    *mlen_p = clen - crypto_aead_chacha20poly1305_ietf_ABYTES;
  if (m && c && mlen_p)
    memcpy(m, c, (size_t)*mlen_p);
  return 0;
}

ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
               const struct sockaddr *dest_addr, socklen_t addrlen) {
  (void)sockfd;
  (void)buf;
  (void)flags;
  (void)dest_addr;
  (void)addrlen;
  sendto_call_count++;
  if (mock_sendto_should_fail) {
    errno = EIO;
    return -1;
  }
  return (ssize_t)len;
}

void _debug(const char *filename, const int linenumber, int level,
            const char *format, ...) {
  (void)filename;
  (void)linenumber;
  (void)level;
  (void)format;
}

void _inform(const char *filename, const int linenumber, const char *format,
             ...) {
  (void)filename;
  (void)linenumber;
  (void)format;
}

void _warn(const char *filename, const int linenumber, const char *format,
           ...) {
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
