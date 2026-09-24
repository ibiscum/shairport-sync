#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <cmocka.h>

#include "common.h"
#include "mdns.h"

extern mdns_backend mdns_avahi;
extern mdns_backend mdns_dns_sd;

extern int mdns_test_die_called;
extern jmp_buf mdns_test_die_jmp;

extern int avahi_register_call_count;
extern int avahi_unregister_call_count;
extern int avahi_monitor_start_call_count;
extern int avahi_monitor_stop_call_count;
extern int avahi_monitor_set_id_call_count;

extern int dns_register_call_count;
extern int dns_unregister_call_count;
extern int dns_monitor_start_call_count;
extern int dns_monitor_stop_call_count;
extern int dns_monitor_set_id_call_count;

extern void mdns_test_reset_stubs(void);
extern void mdns_test_enable_die_trap(int enabled);
extern void mdns_test_set_avahi_register_result(int result);
extern void mdns_test_set_dns_register_result(int result);

static int setup(void **state) {
  (void)state;

  mdns_test_reset_stubs();
  memset(&config, 0, sizeof(config));

  config.service_name = "Unit Test Receiver";
  config.port = 5000;

  config.ap1_prefix[0] = 0x01;
  config.ap1_prefix[1] = 0x02;
  config.ap1_prefix[2] = 0x03;
  config.ap1_prefix[3] = 0x04;
  config.ap1_prefix[4] = 0x05;
  config.ap1_prefix[5] = 0x06;

  return 0;
}

static void assert_register_dies(void) {
  mdns_test_enable_die_trap(1);
  if (setjmp(mdns_test_die_jmp) == 0) {
    mdns_register(NULL, NULL);
    fail_msg("expected mdns_register to die");
  }
  mdns_test_enable_die_trap(0);
}

static void test_named_backend_missing_dies_and_clears_stale_pointer(void **state) {
  (void)state;

  static mdns_backend stale_backend = {.name = "stale"};
  config.mdns = &stale_backend;
  config.mdns_name = "missing-backend";

  assert_register_dies();

  assert_int_equal(mdns_test_die_called, 1);
  assert_null(config.mdns);
  assert_int_equal(avahi_register_call_count, 0);
  assert_int_equal(dns_register_call_count, 0);
}

static void test_named_backend_failure_dies_and_clears_stale_pointer(void **state) {
  (void)state;

  static mdns_backend stale_backend = {.name = "stale"};
  config.mdns = &stale_backend;
  config.mdns_name = "avahi";
  mdns_test_set_avahi_register_result(-1);

  assert_register_dies();

  assert_int_equal(mdns_test_die_called, 1);
  assert_null(config.mdns);
  assert_int_equal(avahi_register_call_count, 1);
}

static void test_default_backend_falls_back_after_failure(void **state) {
  (void)state;

  config.mdns_name = NULL;
  mdns_test_set_avahi_register_result(-1);
  mdns_test_set_dns_register_result(0);

  mdns_register(NULL, NULL);

  assert_ptr_equal(config.mdns, &mdns_dns_sd);
  assert_int_equal(avahi_register_call_count, 1);
  assert_int_equal(dns_register_call_count, 1);
  assert_int_equal(dns_monitor_start_call_count, 1);

  mdns_unregister();

  assert_null(config.mdns);
  assert_int_equal(dns_monitor_stop_call_count, 1);
  assert_int_equal(dns_unregister_call_count, 1);
}

static void test_unregister_clears_pointer_after_success(void **state) {
  (void)state;

  config.mdns_name = "avahi";

  mdns_register(NULL, NULL);

  assert_ptr_equal(config.mdns, &mdns_avahi);
  assert_int_equal(avahi_monitor_start_call_count, 1);

  mdns_unregister();

  assert_null(config.mdns);
  assert_int_equal(avahi_monitor_stop_call_count, 1);
  assert_int_equal(avahi_unregister_call_count, 1);
}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test_setup_teardown(
          test_named_backend_missing_dies_and_clears_stale_pointer, setup, NULL),
      cmocka_unit_test_setup_teardown(
          test_named_backend_failure_dies_and_clears_stale_pointer, setup, NULL),
      cmocka_unit_test_setup_teardown(test_default_backend_falls_back_after_failure, setup, NULL),
      cmocka_unit_test_setup_teardown(test_unregister_clears_pointer_after_success, setup, NULL),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
