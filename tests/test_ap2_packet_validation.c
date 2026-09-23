#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include <cmocka.h>

#include "utilities/ap2_packet_validation.h"

static void test_allocation_succeeded_checks_null(void **state) {
  (void)state;
  int value = 7;
  assert_int_equal(ap2_allocation_succeeded(&value), 1);
  assert_int_equal(ap2_allocation_succeeded(NULL), 0);
}

static void test_buffered_packet_length_rejects_too_small_values(void **state) {
  (void)state;
  size_t packet_size = 99;

  assert_int_equal(ap2_calculate_buffered_audio_packet_size(0, 16384, &packet_size), 0);
  assert_int_equal(packet_size, 99);

  assert_int_equal(ap2_calculate_buffered_audio_packet_size(1, 16384, &packet_size), 0);
  assert_int_equal(packet_size, 99);
}

static void test_buffered_packet_length_rejects_oversize_payload(void **state) {
  (void)state;
  size_t packet_size = 0;

  assert_int_equal(ap2_calculate_buffered_audio_packet_size(16387, 16384, &packet_size), 0);
  assert_int_equal(packet_size, 0);
}

static void test_buffered_packet_length_accepts_boundary_payload(void **state) {
  (void)state;
  size_t packet_size = 0;

  assert_int_equal(ap2_calculate_buffered_audio_packet_size(16386, 16384, &packet_size), 1);
  assert_int_equal(packet_size, 16384);
}

static void test_encrypted_packet_length_rejects_short_packets(void **state) {
  (void)state;

  assert_int_equal(ap2_encrypted_packet_length_is_valid(35), 0);
}

static void test_encrypted_packet_length_accepts_minimum_packet(void **state) {
  (void)state;

  assert_int_equal(ap2_encrypted_packet_length_is_valid(36), 1);
}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test_allocation_succeeded_checks_null),
      cmocka_unit_test(test_buffered_packet_length_rejects_too_small_values),
      cmocka_unit_test(test_buffered_packet_length_rejects_oversize_payload),
      cmocka_unit_test(test_buffered_packet_length_accepts_boundary_payload),
      cmocka_unit_test(test_encrypted_packet_length_rejects_short_packets),
      cmocka_unit_test(test_encrypted_packet_length_accepts_minimum_packet),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
