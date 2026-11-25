#include <errno.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cmocka.h>

#include "activity_monitor.h"
#include "common.h"
#include "test_activity_monitor_stubs.h"

extern enum am_state state;
extern int activity_monitor_running;
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
  activity_monitor_running = 0;

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

static int lifecycle_setup(void **test_state) {
  (void)test_state;

  memset(&config, 0, sizeof(config));
  config.cmd_active_start = "active-start";
  config.cmd_active_stop = "active-stop";
  config.cmd_blocking = 1;
  config.active_state_timeout = 0.0;
  config.disable_standby_mode = disable_standby_auto;

  reset_command_execute_stub();
  activity_monitor_running = 0;
  state = am_inactive;
  return 0;
}

static int lifecycle_teardown(void **test_state) {
  (void)test_state;
  if (activity_monitor_running)
    activity_monitor_stop();
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

static void test_reactivation_cancels_deferred_inactive_transition(void **test_state) {
  (void)test_state;

  activity_monitor_signify_activity(1);
  reset_command_execute_stub();

  config.active_state_timeout = 2.5;
  activity_monitor_signify_activity(0);
  activity_monitor_signify_activity(1);

  assert_int_equal(activity_status(), am_active);
  assert_int_equal(command_execute_call_count, 0);
  assert_int_equal(config.keep_dac_busy, 1);
}

static void test_non_auto_standby_mode_does_not_toggle_keep_dac_busy(void **test_state) {
  (void)test_state;
  config.disable_standby_mode = disable_standby_off;
  config.keep_dac_busy = 7;

  activity_monitor_signify_activity(1);
  assert_int_equal(config.keep_dac_busy, 7);

  activity_monitor_signify_activity(0);
  assert_int_equal(config.keep_dac_busy, 7);
}

static void test_null_command_hooks_are_noops(void **test_state) {
  (void)test_state;
  config.cmd_active_start = NULL;
  config.cmd_active_stop = NULL;

  activity_monitor_signify_activity(1);
  activity_monitor_signify_activity(0);

  assert_int_equal(command_execute_call_count, 0);
}

static void test_start_stop_lifecycle_updates_running_flag(void **test_state) {
  (void)test_state;

  activity_monitor_start();
  assert_int_equal(named_pthread_create_call_count, 1);
  assert_int_equal(activity_monitor_running, 1);

  state = am_inactive;
  activity_monitor_stop();
  assert_int_equal(activity_monitor_running, 0);
}

static void test_stop_from_active_runs_inactive_hook(void **test_state) {
  (void)test_state;

  activity_monitor_start();
  state = am_active;
  reset_command_execute_stub();

  activity_monitor_stop();

  assert_int_equal(activity_monitor_running, 0);
  assert_int_equal(command_execute_call_count, 1);
  assert_string_equal(command_execute_last_command, "active-stop");
}

static void test_start_failure_aborts(void **test_state) {
  (void)test_state;

  pid_t child = fork();
  assert_true(child >= 0);
  if (child == 0) {
    named_pthread_create_return_value = EAGAIN;
    activity_monitor_start();
    _exit(0);
  }

  int status = 0;
  assert_int_equal(waitpid(child, &status, 0), child);
  assert_true(WIFSIGNALED(status));
}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test_setup_teardown(test_inactive_to_active_is_immediate, setup, teardown),
      cmocka_unit_test_setup_teardown(
          test_active_to_inactive_immediate_when_timeout_zero, setup, teardown),
      cmocka_unit_test_setup_teardown(
          test_active_to_inactive_deferred_when_timeout_nonzero, setup, teardown),
      cmocka_unit_test_setup_teardown(
          test_reactivation_cancels_deferred_inactive_transition, setup, teardown),
      cmocka_unit_test_setup_teardown(
          test_non_auto_standby_mode_does_not_toggle_keep_dac_busy, setup, teardown),
      cmocka_unit_test_setup_teardown(test_null_command_hooks_are_noops, setup, teardown),
      cmocka_unit_test_setup_teardown(
          test_start_stop_lifecycle_updates_running_flag, lifecycle_setup, lifecycle_teardown),
      cmocka_unit_test_setup_teardown(
          test_stop_from_active_runs_inactive_hook, lifecycle_setup, lifecycle_teardown),
      cmocka_unit_test_setup_teardown(test_start_failure_aborts, lifecycle_setup,
                                      lifecycle_teardown),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
