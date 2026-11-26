#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <cmocka.h>

#include "metadata/hub.h"
#include "remote/remote.h"
#include "rtsp.h"

extern struct metadata_bundle metadata_store;
extern rtsp_conn_info *principal_conn;

extern int remote_test_send_simple_dacp_command_call_count;
extern char remote_test_last_simple_dacp_command[256];
extern int remote_test_dacp_set_integer_percent_volume_call_count;
extern int remote_test_last_dacp_integer_percent_volume;
extern void remote_test_reset_stubs(void);

static rtsp_conn_info dummy_conn;

static int setup(__attribute__((unused)) void **state) {
  remote_test_reset_stubs();
  memset(&dummy_conn, 0, sizeof(dummy_conn));
  return 0;
}

static void test_remote_simple_command_sends_mapped_when_dacp_active(
    __attribute__((unused)) void **state) {
  metadata_store.dacp_server_active = 1;

  remote_simple_command(rcsc_play_pause);

  assert_int_equal(remote_test_send_simple_dacp_command_call_count, 1);
  assert_string_equal(remote_test_last_simple_dacp_command, "playpause");
}

static void test_remote_simple_command_skips_unmapped_command(
    __attribute__((unused)) void **state) {
  metadata_store.dacp_server_active = 1;

  remote_simple_command(rcsc_cycle_repeat);

  assert_int_equal(remote_test_send_simple_dacp_command_call_count, 0);
}

static void test_remote_set_airplay_volume_uses_dacp_when_active(__attribute__((unused)) void **state) {
  metadata_store.dacp_server_active = 1;

  assert_int_equal(remote_set_airplay_volume(-12.5), 1);

  assert_int_equal(remote_test_send_simple_dacp_command_call_count, 1);
  assert_string_equal(remote_test_last_simple_dacp_command, "setproperty?dmcp.device-volume=-12.500000");
}

static void test_remote_set_integer_percent_volume_uses_dacp_advanced(
    __attribute__((unused)) void **state) {
  metadata_store.advanced_dacp_server_active = 1;

  assert_int_equal(remote_set_integer_percent_volume(42), 1);

  assert_int_equal(remote_test_dacp_set_integer_percent_volume_call_count, 1);
  assert_int_equal(remote_test_last_dacp_integer_percent_volume, 42);
}

static void test_remote_set_repeat_mode_requires_connection(__attribute__((unused)) void **state) {
  principal_conn = NULL;
  metadata_store.advanced_dacp_server_active = 1;

  assert_int_equal(remote_set_repeat_mode(RS_ALL), 0);
  assert_int_equal(remote_test_send_simple_dacp_command_call_count, 0);
}

static void test_remote_set_repeat_mode_dacp_command_mapping(__attribute__((unused)) void **state) {
  principal_conn = &dummy_conn;
  metadata_store.advanced_dacp_server_active = 1;

  assert_int_equal(remote_set_repeat_mode(RS_OFF), 1);
  assert_string_equal(remote_test_last_simple_dacp_command, "setproperty?dacp.repeatstate=0");

  assert_int_equal(remote_set_repeat_mode(RS_ONE), 1);
  assert_string_equal(remote_test_last_simple_dacp_command, "setproperty?dacp.repeatstate=1");

  assert_int_equal(remote_set_repeat_mode(RS_ALL), 1);
  assert_string_equal(remote_test_last_simple_dacp_command, "setproperty?dacp.repeatstate=2");
}

static void test_remote_set_shuffle_mode_requires_connection(__attribute__((unused)) void **state) {
  principal_conn = NULL;
  metadata_store.advanced_dacp_server_active = 1;

  assert_int_equal(remote_set_shuffle_mode(SS_ON), 0);
  assert_int_equal(remote_test_send_simple_dacp_command_call_count, 0);
}

static void test_remote_set_shuffle_mode_dacp_command_mapping(__attribute__((unused)) void **state) {
  principal_conn = &dummy_conn;
  metadata_store.advanced_dacp_server_active = 1;

  assert_int_equal(remote_set_shuffle_mode(SS_OFF), 1);
  assert_string_equal(remote_test_last_simple_dacp_command, "setproperty?dacp.shufflestate=0");

  assert_int_equal(remote_set_shuffle_mode(SS_ON), 1);
  assert_string_equal(remote_test_last_simple_dacp_command, "setproperty?dacp.shufflestate=1");
}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test_setup_teardown(test_remote_simple_command_sends_mapped_when_dacp_active, setup,
                                      NULL),
      cmocka_unit_test_setup_teardown(test_remote_simple_command_skips_unmapped_command, setup,
                                      NULL),
      cmocka_unit_test_setup_teardown(test_remote_set_airplay_volume_uses_dacp_when_active, setup,
                                      NULL),
      cmocka_unit_test_setup_teardown(test_remote_set_integer_percent_volume_uses_dacp_advanced,
                                      setup, NULL),
      cmocka_unit_test_setup_teardown(test_remote_set_repeat_mode_requires_connection, setup, NULL),
      cmocka_unit_test_setup_teardown(test_remote_set_repeat_mode_dacp_command_mapping, setup, NULL),
      cmocka_unit_test_setup_teardown(test_remote_set_shuffle_mode_requires_connection, setup, NULL),
      cmocka_unit_test_setup_teardown(test_remote_set_shuffle_mode_dacp_command_mapping, setup,
                                      NULL),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
