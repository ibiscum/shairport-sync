#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h>

#include "mdns/tinysvcmdns/tinysvcmdns.h"

static void test_create_nlabel_normalizes_ascii_case(void **state) {
  (void)state;

  uint8_t *nlabel = create_nlabel("LiViNg.RoOm.LOCAL");

  assert_non_null(nlabel);
  assert_int_equal(nlabel[0], 6);
  assert_memory_equal(nlabel + 1, "living", 6);
  assert_int_equal(nlabel[7], 4);
  assert_memory_equal(nlabel + 8, "room", 4);
  assert_int_equal(nlabel[12], 5);
  assert_memory_equal(nlabel + 13, "local", 5);
  assert_int_equal(nlabel[18], 0);

  free(nlabel);
}

static void test_cmp_nlabel_is_case_insensitive(void **state) {
  (void)state;

  uint8_t *lhs = create_nlabel("Kitchen.Local");
  uint8_t *rhs = create_nlabel("kitchen.local");

  assert_non_null(lhs);
  assert_non_null(rhs);
  assert_int_equal(cmp_nlabel(lhs, rhs), 0);

  free(lhs);
  free(rhs);
}

static void test_cmp_nlabel_handles_null_inputs(void **state) {
  (void)state;

  uint8_t *name = create_nlabel("zone.local");

  assert_non_null(name);
  assert_int_equal(cmp_nlabel(NULL, NULL), 0);
  assert_true(cmp_nlabel(NULL, name) < 0);
  assert_true(cmp_nlabel(name, NULL) > 0);

  free(name);
}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test_create_nlabel_normalizes_ascii_case),
      cmocka_unit_test(test_cmp_nlabel_is_case_insensitive),
      cmocka_unit_test(test_cmp_nlabel_handles_null_inputs),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
