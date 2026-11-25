#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include <cmocka.h>

#include "mdns/tinysvcmdns/tinysvcmdns.h"

static void test_ptr_name_prefers_explicit_name(void **state) {
  (void)state;

  uint8_t explicit_name[] = "\x03" "foo" "\x05" "local";
  uint8_t entry_name[] = "\x03" "bar" "\x05" "local";

  struct rr_entry target = {0};
  target.name = entry_name;

  struct rr_entry rr = {0};
  rr.type = RR_PTR;
  rr.data.PTR.name = explicit_name;
  rr.data.PTR.entry = &target;

  assert_ptr_equal(mdns_rr_get_ptr_name(&rr), explicit_name);
}

static void test_ptr_name_falls_back_to_entry_name(void **state) {
  (void)state;

  uint8_t entry_name[] = "\x03" "bar" "\x05" "local";

  struct rr_entry target = {0};
  target.name = entry_name;

  struct rr_entry rr = {0};
  rr.type = RR_PTR;
  rr.data.PTR.name = NULL;
  rr.data.PTR.entry = &target;

  assert_ptr_equal(mdns_rr_get_ptr_name(&rr), entry_name);
}

static void test_ptr_name_returns_null_when_missing(void **state) {
  (void)state;

  struct rr_entry rr = {0};
  rr.type = RR_PTR;
  rr.data.PTR.name = NULL;
  rr.data.PTR.entry = NULL;

  assert_null(mdns_rr_get_ptr_name(&rr));
}

static void test_ptr_name_returns_null_for_null_rr(void **state) {
  (void)state;

  assert_null(mdns_rr_get_ptr_name(NULL));
}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test_ptr_name_prefers_explicit_name),
      cmocka_unit_test(test_ptr_name_falls_back_to_entry_name),
      cmocka_unit_test(test_ptr_name_returns_null_when_missing),
      cmocka_unit_test(test_ptr_name_returns_null_for_null_rr),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
