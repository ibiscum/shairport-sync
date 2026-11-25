#include <limits.h>
#include <errno.h>
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

static void test_regression_player_thread_create_failure_is_detected(void **state) {
  (void)state;
  assert_int_equal(player_pthread_create_succeeded(0), 1);
  assert_int_equal(player_pthread_create_succeeded(EAGAIN), 0);
  assert_int_equal(player_pthread_create_succeeded(-1), 0);
}

static void test_regression_player_thread_join_failure_is_detected(void **state) {
  (void)state;
  assert_int_equal(player_pthread_join_succeeded(0), 1);
  assert_int_equal(player_pthread_join_succeeded(EINVAL), 0);
  assert_int_equal(player_pthread_join_succeeded(-1), 0);
}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test_regression_oversized_uncompressed_packet_is_clamped),
      cmocka_unit_test(test_regression_large_frame_params_do_not_wrap),
      cmocka_unit_test(test_regression_player_thread_create_failure_is_detected),
      cmocka_unit_test(test_regression_player_thread_join_failure_is_detected),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}