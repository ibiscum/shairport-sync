#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include <limits.h>

#include <cmocka.h>

#include "utilities/audio_pw_safety.h"

static void test_validate_configuration_accepts_supported_request(void **state) {
  (void)state;
  assert_int_equal(audio_pw_validate_configuration_request(1, 44100, 2), 1);
}

static void test_validate_configuration_rejects_unsupported_cases(void **state) {
  (void)state;
  assert_int_equal(audio_pw_validate_configuration_request(0, 44100, 2), 0);
  assert_int_equal(audio_pw_validate_configuration_request(1, 0, 2), 0);
  assert_int_equal(audio_pw_validate_configuration_request(1, 384001, 2), 0);
  assert_int_equal(audio_pw_validate_configuration_request(1, 44100, 0), 0);
  assert_int_equal(audio_pw_validate_configuration_request(1, 44100, 9), 0);
}

static void test_validate_runtime_handles_reports_missing_dependencies(void **state) {
  (void)state;
  assert_int_equal(audio_pw_validate_runtime_handles(NULL, (void *)1), -ENODEV);
  assert_int_equal(audio_pw_validate_runtime_handles((void *)1, NULL), -ENODEV);
  assert_int_equal(audio_pw_validate_runtime_handles((void *)1, (void *)1), 0);
}

static void test_validate_connect_result_passthrough(void **state) {
  (void)state;
  assert_int_equal(audio_pw_validate_connect_result(0), 0);
  assert_int_equal(audio_pw_validate_connect_result(-5), -5);
}

static void test_configure_retry_semantics_after_connect_failure(void **state) {
  (void)state;
  int32_t requested_format = 0x11223344;

  assert_int_equal(audio_pw_requires_reconfigure(0, requested_format), 1);

  int connect_result = audio_pw_validate_connect_result(-EIO);
  int32_t state_after_failure = audio_pw_next_configured_format(requested_format, connect_result);
  assert_int_equal(state_after_failure, 0);
  assert_int_equal(audio_pw_requires_reconfigure(state_after_failure, requested_format), 1);

  int32_t state_after_success = audio_pw_next_configured_format(requested_format, 0);
  assert_int_equal(state_after_success, requested_format);
  assert_int_equal(audio_pw_requires_reconfigure(state_after_success, requested_format), 0);
}

static void test_delay_math_saturates_underflow(void **state) {
  (void)state;
  uint64_t delay = audio_pw_calculate_delay_frames(2, 3, 4, 5, 40);
  assert_int_equal(delay, 0);
}

static void test_delay_math_normal_case(void **state) {
  (void)state;
  uint64_t delay = audio_pw_calculate_delay_frames(20, 10, 5, 15, 12);
  assert_int_equal(delay, 38);
}

static void test_delay_math_saturates_sum_overflow(void **state) {
  (void)state;
  uint64_t delay = audio_pw_calculate_delay_frames(UINT64_MAX - 2, 10, 10, 10, 1);
  assert_int_equal(delay, UINT64_MAX - 1);
}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test_validate_configuration_accepts_supported_request),
      cmocka_unit_test(test_validate_configuration_rejects_unsupported_cases),
      cmocka_unit_test(test_validate_runtime_handles_reports_missing_dependencies),
      cmocka_unit_test(test_validate_connect_result_passthrough),
      cmocka_unit_test(test_configure_retry_semantics_after_connect_failure),
      cmocka_unit_test(test_delay_math_saturates_underflow),
      cmocka_unit_test(test_delay_math_normal_case),
      cmocka_unit_test(test_delay_math_saturates_sum_overflow),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
