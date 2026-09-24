#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include <cmocka.h>

#include "player_common.h"

static void test_stream_category_names(void **state) {
  (void)state;

  assert_string_equal(player_stream_category_name(unspecified_stream_category),
                      "unspecified stream");
  assert_string_equal(player_stream_category_name(ptp_stream), "PTP stream");
  assert_string_equal(player_stream_category_name(ntp_stream), "NTP stream");
  assert_string_equal(player_stream_category_name(remote_control_stream),
                      "Remote Control stream");
  assert_string_equal(player_stream_category_name(classic_airplay_stream),
                      "Classic AirPlay stream");
}

static void test_safe_copy_short_packet(void **state) {
  (void)state;
  size_t result = player_safe_uncompressed_bytes_to_copy(128, 352, 4);
  assert_int_equal(result, 128);
}

static void test_safe_copy_exact_packet(void **state) {
  (void)state;
  size_t result = player_safe_uncompressed_bytes_to_copy(1408, 352, 4);
  assert_int_equal(result, 1408);
}

static void test_safe_copy_negative_packet(void **state) {
  (void)state;
  size_t result = player_safe_uncompressed_bytes_to_copy(-1, 352, 4);
  assert_int_equal(result, 0);
}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test_stream_category_names),
      cmocka_unit_test(test_safe_copy_short_packet),
      cmocka_unit_test(test_safe_copy_exact_packet),
      cmocka_unit_test(test_safe_copy_negative_packet),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}