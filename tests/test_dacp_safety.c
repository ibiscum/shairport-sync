#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

#include <cmocka.h>

#include "utilities/dacp_safety.h"

static uint32_t make_fourcc(char a, char b, char c, char d) {
  return ((uint32_t)(uint8_t)a << 24) | ((uint32_t)(uint8_t)b << 16) |
         ((uint32_t)(uint8_t)c << 8) | (uint32_t)(uint8_t)d;
}

static void test_safe_copy_null_terminates_truncated_string(void **state) {
  (void)state;
  char dst[6];
  memset(dst, 'X', sizeof(dst));

  dacp_safe_copy_string(dst, sizeof(dst), "abcdefghijklmnop");

  assert_string_equal(dst, "abcde");
  assert_int_equal(dst[5], '\0');
}

static void test_safe_copy_handles_null_source(void **state) {
  (void)state;
  char dst[4] = {'A', 'B', 'C', '\0'};

  dacp_safe_copy_string(dst, sizeof(dst), NULL);

  assert_string_equal(dst, "");
}

static void test_tlv_checked_accepts_valid_record(void **state) {
  (void)state;

  uint8_t payload[] = {
      'c', 'm', 'v', 'o',
      0x00, 0x00, 0x00, 0x04,
      0x00, 0x00, 0x00, 0x2A,
  };

  char *cursor = (char *)payload;
  ssize_t remaining = sizeof(payload);
  uint32_t type = 0;
  int32_t length = 0;

  assert_int_equal(dacp_tlv_crawl_checked(&cursor, &remaining, &type, &length), 0);
  assert_int_equal(type, make_fourcc('c', 'm', 'v', 'o'));
  assert_int_equal(length, 4);
  assert_int_equal(remaining, 0);
}

static void test_tlv_checked_rejects_short_header(void **state) {
  (void)state;

  uint8_t payload[] = {'c', 'm', 'v', 'o', 0x00, 0x00, 0x00};
  char *cursor = (char *)payload;
  ssize_t remaining = sizeof(payload);
  uint32_t type = 0;
  int32_t length = 0;

  assert_int_equal(dacp_tlv_crawl_checked(&cursor, &remaining, &type, &length), -1);
}

static void test_tlv_checked_rejects_payload_overrun(void **state) {
  (void)state;

  uint8_t payload[] = {
      'c', 'm', 'v', 'o',
      0x00, 0x00, 0x00, 0x10,
      0x00, 0x00, 0x00, 0x2A,
  };

  char *cursor = (char *)payload;
  ssize_t remaining = sizeof(payload);
  uint32_t type = 0;
  int32_t length = 0;

  assert_int_equal(dacp_tlv_crawl_checked(&cursor, &remaining, &type, &length), -1);
}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test_safe_copy_null_terminates_truncated_string),
      cmocka_unit_test(test_safe_copy_handles_null_source),
      cmocka_unit_test(test_tlv_checked_accepts_valid_record),
      cmocka_unit_test(test_tlv_checked_rejects_short_header),
      cmocka_unit_test(test_tlv_checked_rejects_payload_overrun),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
