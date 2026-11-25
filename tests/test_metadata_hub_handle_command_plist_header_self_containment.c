#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h>
#include "metadata/metadata_hub_handle_command_plist.h"

static void test_metadata_hub_handle_command_plist_header_self_containment_compiles(
    __attribute__((unused)) void **state) {}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test_metadata_hub_handle_command_plist_header_self_containment_compiles),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
