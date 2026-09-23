#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>

#include <cmocka.h>

#include "activity_monitor.h"
#include "common.h"
#include "test_activity_monitor_stubs.h"

extern enum am_state state;
extern int player_state;
extern pthread_mutex_t activity_monitor_mutex;
extern pthread_cond_t activity_monitor_cv;
extern int activity_monitor_running;
extern void *activity_monitor_thread_code(void *arg);

static int setup(void **test_state) {
  (void)test_state;

  memset(&config, 0, sizeof(config));
  config.cmd_active_start = "active-start";
  config.cmd_active_stop = "active-stop";
  config.cmd_blocking = 1;
  config.active_state_timeout = 0.0;
  config.disable_standby_mode = disable_standby_auto;

  reset_command_execute_stub();
  reset_thread_lifecycle_stubs();

  assert_int_equal(pthread_mutex_init(&activity_monitor_mutex, NULL), 0);
  assert_int_equal(pthread_cond_init(&activity_monitor_cv, NULL), 0);

  state = am_inactive;
  activity_monitor_running = 0;
  return 0;
}

static int teardown(void **test_state) {
  (void)test_state;
  pthread_cond_destroy(&activity_monitor_cv);
  pthread_mutex_destroy(&activity_monitor_mutex);
  return 0;
}

static int setup_threaded(void **test_state) {
  (void)test_state;

  memset(&config, 0, sizeof(config));
  config.cmd_active_start = "active-start";
  config.cmd_active_stop = "active-stop";
  config.cmd_blocking = 1;
  config.active_state_timeout = 0.03;
  config.disable_standby_mode = disable_standby_auto;

  reset_command_execute_stub();
  reset_thread_lifecycle_stubs();
  set_named_pthread_create_real_mode(1);

  state = am_inactive;
  activity_monitor_running = 0;
  return 0;
}

static int teardown_threaded(void **test_state) {
  (void)test_state;
  set_named_pthread_create_real_mode(0);
  return 0;
}

static int wait_for_command_calls(int expected, int timeout_ms) {
  int waited_ms = 0;
  while (waited_ms < timeout_ms) {
    if (command_execute_call_count >= expected)
      return 1;
    usleep(1000);
    waited_ms++;
  }
  return 0;
}

static int wait_for_state(enum am_state expected_state, int timeout_ms) {
  int waited_ms = 0;
  while (waited_ms < timeout_ms) {
    if (activity_status() == expected_state)
      return 1;
    usleep(1000);
    waited_ms++;
  }
  return 0;
}

static int wait_for_monitor_ready(int timeout_ms) {
  int waited_ms = 0;
  while (waited_ms < timeout_ms) {
    if (pthread_mutex_trylock(&activity_monitor_mutex) == 0) {
      pthread_mutex_unlock(&activity_monitor_mutex);
      return 1;
    }
    usleep(1000);
    waited_ms++;
  }
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

static void test_inactive_signal_while_inactive_is_noop(void **test_state) {
  (void)test_state;

  activity_monitor_signify_activity(0);

  assert_int_equal(activity_status(), am_inactive);
  assert_int_equal(command_execute_call_count, 0);
}

static void test_activity_monitor_start_marks_running_and_creates_thread(void **test_state) {
  (void)test_state;

  activity_monitor_start();

  assert_int_equal(activity_monitor_running, 1);
  assert_int_equal(named_pthread_create_call_count, 1);
}

static void test_activity_monitor_stop_when_not_running_is_noop(void **test_state) {
  (void)test_state;

  state = am_active;
  activity_monitor_running = 0;

  activity_monitor_stop();

  assert_int_equal(command_execute_call_count, 0);
}

static void test_timing_out_branch_transitions_to_inactive_in_thread(void **test_state) {
  (void)test_state;

  activity_monitor_start();
  assert_int_equal(named_pthread_create_call_count, 1);

  assert_true(wait_for_monitor_ready(500));

  reset_command_execute_stub();

  /* Move the worker from am_inactive into am_active deterministically. */
  assert_int_equal(pthread_mutex_lock(&activity_monitor_mutex), 0);
  state = am_active;
  player_state = 1; /* ps_active */
  assert_int_equal(pthread_cond_signal(&activity_monitor_cv), 0);
  assert_int_equal(pthread_mutex_unlock(&activity_monitor_mutex), 0);

  /* Let it settle into am_active waiting for player_state to go inactive. */
  usleep(20000);

  assert_int_equal(pthread_mutex_lock(&activity_monitor_mutex), 0);
  player_state = 0; /* ps_inactive */
  assert_int_equal(pthread_cond_signal(&activity_monitor_cv), 0);
  assert_int_equal(pthread_mutex_unlock(&activity_monitor_mutex), 0);

  assert_true(wait_for_state(am_inactive, 500));
  assert_true(wait_for_command_calls(1, 300));
  assert_string_equal(command_execute_last_command, "active-stop");
  assert_int_equal(command_execute_last_block, 0);
  assert_int_equal(activity_status(), am_inactive);

  activity_monitor_stop();
}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test_setup_teardown(test_inactive_to_active_is_immediate, setup, teardown),
      cmocka_unit_test_setup_teardown(
          test_active_to_inactive_immediate_when_timeout_zero, setup, teardown),
      cmocka_unit_test_setup_teardown(
          test_active_to_inactive_deferred_when_timeout_nonzero, setup, teardown),
      cmocka_unit_test_setup_teardown(test_inactive_signal_while_inactive_is_noop, setup,
                                      teardown),
      cmocka_unit_test_setup_teardown(
          test_activity_monitor_start_marks_running_and_creates_thread, setup, teardown),
      cmocka_unit_test_setup_teardown(
          test_activity_monitor_stop_when_not_running_is_noop, setup, teardown),
        cmocka_unit_test_setup_teardown(
          test_timing_out_branch_transitions_to_inactive_in_thread, setup_threaded,
          teardown_threaded),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
