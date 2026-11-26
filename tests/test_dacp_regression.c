#include <arpa/inet.h>
#include <errno.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <cmocka.h>

#include "dacp.h"

extern int dacp_monitor_initialised;
extern int dacp_named_pthread_create_calls;
extern void dacp_test_reset_stubs(void);
extern void dacp_test_set_named_pthread_create_return_value(int value);

static uint32_t make_fourcc(char a, char b, char c, char d) {
  return ((uint32_t)(uint8_t)a << 24) | ((uint32_t)(uint8_t)b << 16) |
         ((uint32_t)(uint8_t)c << 8) | (uint32_t)(uint8_t)d;
}

static int teardown_monitor(void **state) {
  (void)state;
  if (dacp_monitor_initialised)
    dacp_monitor_stop();
  return 0;
}

static int setup_monitor(void **state) {
  (void)state;
  dacp_test_reset_stubs();
  dacp_test_set_named_pthread_create_return_value(0);
  if (dacp_monitor_initialised)
    dacp_monitor_stop();
  dacp_monitor_start();
  assert_int_equal(dacp_monitor_initialised, 1);
  return 0;
}

static void test_tlv_crawl_accepts_valid_item(void **state) {
  (void)state;
  uint8_t payload[] = {
      'c', 'm', 'v', 'o',
      0x00, 0x00, 0x00, 0x04,
      0x00, 0x00, 0x00, 0x2A,
  };

  char *cursor = (char *)payload;
  size_t item_size = 0;

  assert_int_equal(dacp_tlv_crawl(&cursor, sizeof(payload), &item_size),
                   make_fourcc('c', 'm', 'v', 'o'));
  assert_int_equal(item_size, 4);
  assert_ptr_equal(cursor, (char *)payload + sizeof(payload));
}

static void test_tlv_crawl_rejects_short_header(void **state) {
  (void)state;
  uint8_t payload[] = {'c', 'm', 'v', 'o', 0x00, 0x00, 0x00};
  char *cursor = (char *)payload;
  size_t item_size = 777;

  assert_int_equal(dacp_tlv_crawl(&cursor, sizeof(payload), &item_size), 0);
  assert_int_equal(item_size, 0);
  assert_ptr_equal(cursor, (char *)payload);
}

static void test_tlv_crawl_rejects_length_overrun(void **state) {
  (void)state;
  uint8_t payload[] = {
      'c', 'm', 'v', 'o',
      0x00, 0x00, 0x00, 0x10,
      0x00, 0x00, 0x00, 0x2A,
  };
  char *cursor = (char *)payload;
  size_t item_size = 999;

  assert_int_equal(dacp_tlv_crawl(&cursor, sizeof(payload), &item_size), 0);
  assert_int_equal(item_size, 0);
  assert_ptr_equal(cursor, (char *)payload);
}

static void test_monitor_start_stop_idempotent(void **state) {
  (void)state;
  dacp_test_reset_stubs();

  dacp_monitor_start();
  assert_int_equal(dacp_monitor_initialised, 1);
  assert_int_equal(dacp_named_pthread_create_calls, 1);

  dacp_monitor_start();
  assert_int_equal(dacp_monitor_initialised, 1);
  assert_int_equal(dacp_named_pthread_create_calls, 1);

  dacp_monitor_stop();
  assert_int_equal(dacp_monitor_initialised, 0);

  dacp_monitor_stop();
  assert_int_equal(dacp_monitor_initialised, 0);
}

static void test_monitor_start_failure_recovers_on_retry(void **state) {
  (void)state;
  dacp_test_reset_stubs();

  dacp_test_set_named_pthread_create_return_value(EAGAIN);
  dacp_monitor_start();
  assert_int_equal(dacp_monitor_initialised, 0);
  assert_int_equal(dacp_named_pthread_create_calls, 1);

  dacp_test_set_named_pthread_create_return_value(0);
  dacp_monitor_start();
  assert_int_equal(dacp_monitor_initialised, 1);
  assert_int_equal(dacp_named_pthread_create_calls, 2);
}

static void test_port_update_requires_matching_dacp_id(void **state) {
  (void)state;
  rtsp_conn_info conn;
  memset(&conn, 0, sizeof(conn));
  conn.connection_number = 17;
  conn.connection_ip_family = AF_INET;
  strncpy(conn.client_ip_string, "bad::ip::literal", sizeof(conn.client_ip_string) - 1);
  conn.client_ip_string[sizeof(conn.client_ip_string) - 1] = '\0';
  conn.dacp_id = "session-a";
  conn.dacp_active_remote = "active-remote-a";

  set_dacp_server_information(&conn);
  assert_int_equal(send_simple_dacp_command("playpause"), 490);

  dacp_monitor_port_update_callback("session-b", 1234);
  assert_int_equal(send_simple_dacp_command("playpause"), 490);

  dacp_monitor_port_update_callback("session-a", 1234);
  assert_int_equal(send_simple_dacp_command("playpause"), 498);
}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test_tlv_crawl_accepts_valid_item),
      cmocka_unit_test(test_tlv_crawl_rejects_short_header),
      cmocka_unit_test(test_tlv_crawl_rejects_length_overrun),
      cmocka_unit_test_setup_teardown(test_monitor_start_stop_idempotent, NULL, teardown_monitor),
      cmocka_unit_test_setup_teardown(test_monitor_start_failure_recovers_on_retry, NULL,
                                      teardown_monitor),
      cmocka_unit_test_setup_teardown(test_port_update_requires_matching_dacp_id, setup_monitor,
                                      teardown_monitor),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
