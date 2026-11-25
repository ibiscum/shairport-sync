#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h>
#ifdef CONFIG_AIRPLAY_2
#undef CONFIG_AIRPLAY_2
#endif

#include "ap2_event_receiver.h"

static void test_ap2_event_receiver_header_self_containment_no_ap2_compiles(
    __attribute__((unused)) void **state) {}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test_ap2_event_receiver_header_self_containment_no_ap2_compiles),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
