#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h>

#include "metadata/core.h"

static void test_metadata_fourcc_valid_string(void **state) {
  (void)state;
  uint32_t code = 0;
  assert_int_equal(metadata_fourcc_from_string("abrt", &code), 0);
  assert_int_equal(code, METADATA_FOURCC('a', 'b', 'r', 't'));
}

static void test_metadata_fourcc_rejects_bad_length(void **state) {
  (void)state;
  uint32_t code = 0;
  assert_int_equal(metadata_fourcc_from_string("abc", &code), -1);
  assert_int_equal(metadata_fourcc_from_string("abcde", &code), -1);
}

static void test_metadata_fourcc_rejects_invalid_characters(void **state) {
  (void)state;
  uint32_t code = 0;
  const char control_char[] = {'a', 'b', '\n', 't', '\0'};
  const char high_bit_char[] = {'a', 'b', (char)0x80, 't', '\0'};
  assert_int_equal(metadata_fourcc_from_string(control_char, &code), -1);
  assert_int_equal(metadata_fourcc_from_string(high_bit_char, &code), -1);
}

static void test_metadata_fourcc_rejects_null_inputs(void **state) {
  (void)state;
  uint32_t code = 0;
  assert_int_equal(metadata_fourcc_from_string(NULL, &code), -1);
  assert_int_equal(metadata_fourcc_from_string("abrt", NULL), -1);
}

static void test_metadata_fourcc_accepts_ascii_boundaries(void **state) {
  (void)state;
  uint32_t code = 0;
  const char boundaries[] = {' ', '~', 'A', 'z', '\0'};
  assert_int_equal(metadata_fourcc_from_string(boundaries, &code), 0);
  assert_int_equal(code, METADATA_FOURCC(' ', '~', 'A', 'z'));
}

static void test_metadata_fourcc_failure_keeps_output_unchanged(void **state) {
  (void)state;
  uint32_t code = 0x11223344;
  assert_int_equal(metadata_fourcc_from_string("abc", &code), -1);
  assert_int_equal(code, 0x11223344);
}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test_metadata_fourcc_valid_string),
      cmocka_unit_test(test_metadata_fourcc_rejects_bad_length),
      cmocka_unit_test(test_metadata_fourcc_rejects_invalid_characters),
      cmocka_unit_test(test_metadata_fourcc_rejects_null_inputs),
      cmocka_unit_test(test_metadata_fourcc_accepts_ascii_boundaries),
      cmocka_unit_test(test_metadata_fourcc_failure_keeps_output_unchanged),
  };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
