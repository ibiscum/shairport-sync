#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include <cmocka.h>

#include "utilities/dacp_safety.h"

static void test_monitor_stop_transition_is_idempotent(void **state) {
  (void)state;
  int monitor_initialised = 1;

  assert_int_equal(dacp_monitor_transition_to_stopped(&monitor_initialised), 1);
  assert_int_equal(monitor_initialised, 0);

  assert_int_equal(dacp_monitor_transition_to_stopped(&monitor_initialised), 0);
  assert_int_equal(monitor_initialised, 0);
}

static void test_monitor_stop_transition_rejects_null_state(void **state) {
  (void)state;
  assert_int_equal(dacp_monitor_transition_to_stopped(NULL), 0);
}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test_monitor_stop_transition_is_idempotent),
      cmocka_unit_test(test_monitor_stop_transition_rejects_null_state),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
