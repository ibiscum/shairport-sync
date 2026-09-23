#include <arpa/inet.h>
#include <pthread.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <cmocka.h>

#include "utilities/ap2_packet_validation.h"
#include "utilities/buffered_read.h"

typedef struct {
  buffered_tcp_desc descriptor;
  uint8_t *storage;
  size_t storage_size;
} buffered_fixture;

static int init_buffered_fixture(buffered_fixture *fixture, const uint8_t *bytes, size_t length) {
  memset(fixture, 0, sizeof(*fixture));

  size_t allocation_size = length == 0 ? 1 : length;
  fixture->storage = malloc(allocation_size);
  if (fixture->storage == NULL)
    return -1;

  fixture->storage_size = allocation_size;
  if (length > 0)
    memcpy(fixture->storage, bytes, length);

  fixture->descriptor.buffer = (char *)fixture->storage;
  fixture->descriptor.buffer_max_size = fixture->storage_size;
  fixture->descriptor.toq = (char *)fixture->storage;
  fixture->descriptor.eoq = (char *)fixture->storage + length;
  fixture->descriptor.buffer_occupancy = length;

  if (pthread_mutex_init(&fixture->descriptor.mutex, NULL) != 0)
    return -1;
  if (pthread_cond_init(&fixture->descriptor.not_empty_cv, NULL) != 0)
    return -1;
  if (pthread_cond_init(&fixture->descriptor.not_full_cv, NULL) != 0)
    return -1;

  return 0;
}

static void destroy_buffered_fixture(buffered_fixture *fixture) {
  pthread_cond_destroy(&fixture->descriptor.not_full_cv);
  pthread_cond_destroy(&fixture->descriptor.not_empty_cv);
  pthread_mutex_destroy(&fixture->descriptor.mutex);
  free(fixture->storage);
}

static void test_valid_framed_block_passes_decrypt_gate(void **state) {
  (void)state;
  uint8_t stream[2 + 40] = {0};
  uint16_t data_len_network = htons(42);
  size_t bytes_remaining = 0;

  memcpy(stream, &data_len_network, sizeof(data_len_network));
  memset(stream + 2, 0xA5, 40);

  buffered_fixture fixture;
  assert_int_equal(init_buffered_fixture(&fixture, stream, sizeof(stream)), 0);

  uint16_t data_len = 0;
  ssize_t nread = read_sized_block(&fixture.descriptor, &data_len, sizeof(data_len),
                                   &bytes_remaining);
  assert_int_equal(nread, sizeof(data_len));
  assert_int_equal(ntohs(data_len), 42);

  size_t packet_size = 0;
  assert_int_equal(ap2_calculate_buffered_audio_packet_size(ntohs(data_len), 16 * 1024,
                                                             &packet_size),
                   1);
  assert_int_equal(packet_size, 40);

  uint8_t packet[16 * 1024] = {0};
  nread = read_sized_block(&fixture.descriptor, packet, packet_size, &bytes_remaining);
  assert_int_equal(nread, 40);
  assert_int_equal(bytes_remaining, 0);

  assert_int_equal(ap2_encrypted_packet_length_is_valid(nread), 1);

  destroy_buffered_fixture(&fixture);
}

static void test_short_encrypted_payload_fails_decrypt_gate(void **state) {
  (void)state;
  uint8_t stream[2 + 34] = {0};
  uint16_t data_len_network = htons(36);
  size_t bytes_remaining = 0;

  memcpy(stream, &data_len_network, sizeof(data_len_network));
  memset(stream + 2, 0x5A, 34);

  buffered_fixture fixture;
  assert_int_equal(init_buffered_fixture(&fixture, stream, sizeof(stream)), 0);

  uint16_t data_len = 0;
  ssize_t nread = read_sized_block(&fixture.descriptor, &data_len, sizeof(data_len),
                                   &bytes_remaining);
  assert_int_equal(nread, sizeof(data_len));

  size_t packet_size = 0;
  assert_int_equal(ap2_calculate_buffered_audio_packet_size(ntohs(data_len), 16 * 1024,
                                                             &packet_size),
                   1);
  assert_int_equal(packet_size, 34);

  uint8_t packet[16 * 1024] = {0};
  nread = read_sized_block(&fixture.descriptor, packet, packet_size, &bytes_remaining);
  assert_int_equal(nread, 34);
  assert_int_equal(ap2_encrypted_packet_length_is_valid(nread), 0);

  destroy_buffered_fixture(&fixture);
}

static void test_too_small_data_len_is_rejected_before_payload_read(void **state) {
  (void)state;
  uint8_t stream[2] = {0};
  uint16_t data_len_network = htons(1);
  size_t bytes_remaining = 0;

  memcpy(stream, &data_len_network, sizeof(data_len_network));

  buffered_fixture fixture;
  assert_int_equal(init_buffered_fixture(&fixture, stream, sizeof(stream)), 0);

  uint16_t data_len = 0;
  ssize_t nread = read_sized_block(&fixture.descriptor, &data_len, sizeof(data_len),
                                   &bytes_remaining);
  assert_int_equal(nread, sizeof(data_len));

  size_t packet_size = 99;
  assert_int_equal(ap2_calculate_buffered_audio_packet_size(ntohs(data_len), 16 * 1024,
                                                             &packet_size),
                   0);
  assert_int_equal(packet_size, 99);
  assert_int_equal(bytes_remaining, 0);

  destroy_buffered_fixture(&fixture);
}

static void test_oversize_data_len_is_rejected_before_payload_read(void **state) {
  (void)state;
  uint8_t stream[2] = {0};
  uint16_t data_len_network = htons(16387);
  size_t bytes_remaining = 0;

  memcpy(stream, &data_len_network, sizeof(data_len_network));

  buffered_fixture fixture;
  assert_int_equal(init_buffered_fixture(&fixture, stream, sizeof(stream)), 0);

  uint16_t data_len = 0;
  ssize_t nread = read_sized_block(&fixture.descriptor, &data_len, sizeof(data_len),
                                   &bytes_remaining);
  assert_int_equal(nread, sizeof(data_len));

  size_t packet_size = 0;
  assert_int_equal(ap2_calculate_buffered_audio_packet_size(ntohs(data_len), 16 * 1024,
                                                             &packet_size),
                   0);
  assert_int_equal(bytes_remaining, 0);

  destroy_buffered_fixture(&fixture);
}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test_valid_framed_block_passes_decrypt_gate),
      cmocka_unit_test(test_short_encrypted_payload_fails_decrypt_gate),
      cmocka_unit_test(test_too_small_data_len_is_rejected_before_payload_read),
      cmocka_unit_test(test_oversize_data_len_is_rejected_before_payload_read),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
