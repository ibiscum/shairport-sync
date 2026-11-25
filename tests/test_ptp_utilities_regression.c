#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include <cmocka.h>

#include <sys/mman.h>

#include "ptp-utilities.h"

extern void *mapped_addr;
extern shairport_cfg config;

extern int ptp_test_getaddrinfo_calls;
extern int ptp_test_socket_calls;
extern int ptp_test_sendto_calls;
void reset_ptp_utilities_stubs(void);

static int setup(void **state) {
  (void)state;
  reset_ptp_utilities_stubs();
  mapped_addr = NULL;
  return 0;
}

static void test_close_is_idempotent_when_already_closed(void **state) {
  (void)state;

  assert_int_equal(ptp_shm_interface_close(), 0);
  assert_null(mapped_addr);
}

static void test_close_treats_map_failed_as_already_closed(void **state) {
  (void)state;

  mapped_addr = MAP_FAILED;
  assert_int_equal(ptp_shm_interface_close(), 0);
  assert_null(mapped_addr);
}

static void test_open_is_idempotent_when_already_open(void **state) {
  (void)state;
  void *sentinel = (void *)0x1;

  mapped_addr = sentinel;
  assert_int_equal(ptp_shm_interface_open(), 0);
  assert_ptr_equal(mapped_addr, sentinel);
  mapped_addr = NULL;
}

static void test_send_control_message_missing_interface_name_is_best_effort(void **state) {
  (void)state;

  ptp_send_control_message_string("P");
  assert_int_equal(ptp_test_getaddrinfo_calls, 0);
  assert_int_equal(ptp_test_socket_calls, 0);
  assert_int_equal(ptp_test_sendto_calls, 0);
}

static void test_send_control_message_empty_message_is_best_effort(void **state) {
  (void)state;

  config.nqptp_shared_memory_interface_name = "/nqptp-test";
  ptp_send_control_message_string("");

  assert_int_equal(ptp_test_getaddrinfo_calls, 0);
  assert_int_equal(ptp_test_socket_calls, 0);
  assert_int_equal(ptp_test_sendto_calls, 0);
}

static void test_send_control_message_null_message_is_best_effort(void **state) {
  (void)state;

  config.nqptp_shared_memory_interface_name = "/nqptp-test";
  ptp_send_control_message_string(NULL);

  assert_int_equal(ptp_test_getaddrinfo_calls, 0);
  assert_int_equal(ptp_test_socket_calls, 0);
  assert_int_equal(ptp_test_sendto_calls, 0);
}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test_setup(test_close_is_idempotent_when_already_closed, setup),
      cmocka_unit_test_setup(test_close_treats_map_failed_as_already_closed, setup),
      cmocka_unit_test_setup(test_open_is_idempotent_when_already_open, setup),
      cmocka_unit_test_setup(test_send_control_message_missing_interface_name_is_best_effort,
                             setup),
      cmocka_unit_test_setup(test_send_control_message_empty_message_is_best_effort, setup),
      cmocka_unit_test_setup(test_send_control_message_null_message_is_best_effort, setup),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
