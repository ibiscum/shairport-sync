#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h>
#include "alac.h"

static void test_alac_header_self_containment_compiles(__attribute__((unused)) void **state) {}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test_alac_header_self_containment_compiles),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
