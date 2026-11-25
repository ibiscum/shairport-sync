#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include <cmocka.h>

#include "nqptp-shm-structures.h"

static void test_shm_structure_set_layout_is_stable(void **state) {
  (void)state;

  assert_int_equal(sizeof(shm_structure_set), 32);
  assert_int_equal(sizeof(nqptp_shm_structure_set), sizeof(shm_structure_set));
}

static void test_shm_structure_layout_is_stable(void **state) {
  (void)state;

  assert_int_equal(offsetof(struct shm_structure, version), 0);
  assert_int_equal(offsetof(struct shm_structure, main), 8);
  assert_int_equal(offsetof(struct shm_structure, secondary), 40);
  assert_int_equal(sizeof(struct shm_structure), 72);
  assert_int_equal(sizeof(nqptp_shm_structure), sizeof(struct shm_structure));
}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test_shm_structure_set_layout_is_stable),
      cmocka_unit_test(test_shm_structure_layout_is_stable),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
