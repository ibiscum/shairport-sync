#include <limits.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include <cmocka.h>

#include "player_common.h"

static void test_regression_oversized_uncompressed_packet_is_clamped(void **state) {
  (void)state;
  size_t result = player_safe_uncompressed_bytes_to_copy(5000, 352, 4);
  assert_int_equal(result, 1408);
}

static void test_regression_large_frame_params_do_not_wrap(void **state) {
  (void)state;
  size_t result =
      player_safe_uncompressed_bytes_to_copy(INT_MAX, UINT_MAX, UINT_MAX);
  assert_int_equal(result, INT_MAX);
}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test_regression_oversized_uncompressed_packet_is_clamped),
      cmocka_unit_test(test_regression_large_frame_params_do_not_wrap),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}