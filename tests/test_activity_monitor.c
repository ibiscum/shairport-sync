#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>

#include <cmocka.h>

#include "activity_monitor.h"
#include "common.h"
#include "test_activity_monitor_stubs.h"

extern enum am_state state;
extern pthread_mutex_t activity_monitor_mutex;
extern pthread_cond_t activity_monitor_cv;

static int setup(void **test_state) {
  (void)test_state;

  memset(&config, 0, sizeof(config));
  config.cmd_active_start = "active-start";
  config.cmd_active_stop = "active-stop";
  config.cmd_blocking = 1;
  config.active_state_timeout = 0.0;
  config.disable_standby_mode = disable_standby_auto;

  reset_command_execute_stub();

  assert_int_equal(pthread_mutex_init(&activity_monitor_mutex, NULL), 0);
  assert_int_equal(pthread_cond_init(&activity_monitor_cv, NULL), 0);

  state = am_inactive;
  return 0;
}

static int teardown(void **test_state) {
  (void)test_state;
  pthread_cond_destroy(&activity_monitor_cv);
  pthread_mutex_destroy(&activity_monitor_mutex);
  return 0;
}

static void test_inactive_to_active_is_immediate(void **test_state) {
  (void)test_state;

  activity_monitor_signify_activity(1);

  assert_int_equal(activity_status(), am_active);
  assert_int_equal(command_execute_call_count, 1);
  assert_string_equal(command_execute_last_command, "active-start");
  assert_int_equal(command_execute_last_block, 1);
  assert_int_equal(config.keep_dac_busy, 1);
}

static void test_active_to_inactive_immediate_when_timeout_zero(void **test_state) {
  (void)test_state;

  activity_monitor_signify_activity(1);
  reset_command_execute_stub();

  activity_monitor_signify_activity(0);

  assert_int_equal(activity_status(), am_inactive);
  assert_int_equal(command_execute_call_count, 1);
  assert_string_equal(command_execute_last_command, "active-stop");
  assert_int_equal(command_execute_last_block, 1);
  assert_int_equal(config.keep_dac_busy, 0);
}

static void test_active_to_inactive_deferred_when_timeout_nonzero(void **test_state) {
  (void)test_state;

  activity_monitor_signify_activity(1);
  reset_command_execute_stub();

  config.active_state_timeout = 2.5;

  activity_monitor_signify_activity(0);

  assert_int_equal(activity_status(), am_active);
  assert_int_equal(command_execute_call_count, 0);
  assert_int_equal(config.keep_dac_busy, 1);
}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test_setup_teardown(test_inactive_to_active_is_immediate, setup, teardown),
      cmocka_unit_test_setup_teardown(
          test_active_to_inactive_immediate_when_timeout_zero, setup, teardown),
      cmocka_unit_test_setup_teardown(
          test_active_to_inactive_deferred_when_timeout_nonzero, setup, teardown),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
