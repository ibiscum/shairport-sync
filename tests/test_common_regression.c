#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h>

#include <stdlib.h>
#include <string.h>

#include "common.h"

static void test_service_type_roundtrip_preserves_forced_classic(void **state) {
  (void)state;
  char text[32];

  service_type_to_string(APST_forced_classic, text);
  assert_string_equal(text, "forced_classic");
  assert_int_equal(string_to_service_type(text, "service_type"), APST_forced_classic);
}

static void test_string_update_with_size_handles_null_flag(void **state) {
  (void)state;
  char *value = NULL;
  int changed = -1;

  assert_int_equal(string_update_with_size(&value, &changed, "abc", 3), 1);
  assert_non_null(value);
  assert_string_equal(value, "abc");

  assert_int_equal(string_update_with_size(&value, NULL, "xyz", 3), 0);
  assert_non_null(value);
  assert_string_equal(value, "xyz");

  free(value);
}

static void test_next_free_udp_port_wrap_skips_reserved_triplet(void **state) {
  (void)state;
  config.udp_port_base = 6000;
  config.udp_port_range = 5;

  resetFreeUDPPort();
  assert_int_equal(nextFreeUDPPort(), 6000);
  assert_int_equal(nextFreeUDPPort(), 6001);
  assert_int_equal(nextFreeUDPPort(), 6002);
  assert_int_equal(nextFreeUDPPort(), 6003);
  assert_int_equal(nextFreeUDPPort(), 6004);
  assert_int_equal(nextFreeUDPPort(), 6003);
}

static void test_vol2attn_out_of_range_is_clamped(void **state) {
  (void)state;
  assert_true(vol2attn(-144.0, 0, -6000) == -6000);
  assert_true(flat_vol2attn(-144.0, 0, -6000) == -6000);
  assert_true(dasl_tapered_vol2attn(-144.0, 0, -6000) == -6000);
}

static void test_encoded_format_macros_roundtrip_nominal_values(void **state) {
  (void)state;
  uint32_t encoded = CHANNELS_TO_ENCODED_FORMAT(2) | RATE_TO_ENCODED_FORMAT(44100) |
                     FORMAT_TO_ENCODED_FORMAT(SPS_FORMAT_S16_LE);

  assert_int_equal(CHANNELS_FROM_ENCODED_FORMAT(encoded), 2);
  assert_int_equal(RATE_FROM_ENCODED_FORMAT(encoded), 44100);
  assert_int_equal(FORMAT_FROM_ENCODED_FORMAT(encoded), SPS_FORMAT_S16_LE);
}

static void test_encoded_format_macros_roundtrip_max_representable_values(void **state) {
  (void)state;
  uint32_t encoded =
      CHANNELS_TO_ENCODED_FORMAT(127) | RATE_TO_ENCODED_FORMAT(1048574) | FORMAT_TO_ENCODED_FORMAT(63);

  assert_int_equal(CHANNELS_FROM_ENCODED_FORMAT(encoded), 127);
  assert_int_equal(RATE_FROM_ENCODED_FORMAT(encoded), 1048574);
  assert_int_equal(FORMAT_FROM_ENCODED_FORMAT(encoded), 63);
}

static void test_encoded_format_macros_mask_and_truncate_inputs(void **state) {
  (void)state;
  uint32_t encoded = CHANNELS_TO_ENCODED_FORMAT(255) | RATE_TO_ENCODED_FORMAT(44101) |
                     FORMAT_TO_ENCODED_FORMAT(0xFF);

  assert_int_equal(CHANNELS_FROM_ENCODED_FORMAT(encoded), 127);
  assert_int_equal(RATE_FROM_ENCODED_FORMAT(encoded), 44100);
  assert_int_equal(FORMAT_FROM_ENCODED_FORMAT(encoded), 63);
}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test_service_type_roundtrip_preserves_forced_classic),
      cmocka_unit_test(test_string_update_with_size_handles_null_flag),
      cmocka_unit_test(test_next_free_udp_port_wrap_skips_reserved_triplet),
      cmocka_unit_test(test_vol2attn_out_of_range_is_clamped),
      cmocka_unit_test(test_encoded_format_macros_roundtrip_nominal_values),
      cmocka_unit_test(test_encoded_format_macros_roundtrip_max_representable_values),
      cmocka_unit_test(test_encoded_format_macros_mask_and_truncate_inputs),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}

int _safe_socket_close(__attribute__((unused)) const char *filename,
                       __attribute__((unused)) const int linenumber, int *sockfd) {
  if (sockfd)
    *sockfd = -1;
  return 0;
}

void _debug(__attribute__((unused)) const char *filename, __attribute__((unused)) const int linenumber,
            __attribute__((unused)) int level, __attribute__((unused)) const char *format, ...) {}

void _warn(__attribute__((unused)) const char *filename, __attribute__((unused)) const int linenumber,
           __attribute__((unused)) const char *format, ...) {}

void _inform(__attribute__((unused)) const char *filename,
             __attribute__((unused)) const int linenumber,
             __attribute__((unused)) const char *format, ...) {}

void _debug_print_buffer(__attribute__((unused)) const char *thefilename,
                         __attribute__((unused)) const int linenumber,
                         __attribute__((unused)) int level, __attribute__((unused)) void *buf,
                         __attribute__((unused)) size_t buf_len) {}

void _die(__attribute__((unused)) const char *filename, __attribute__((unused)) const int linenumber,
          __attribute__((unused)) const char *format, ...) {
  abort();
}
