#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <string.h>

#include <cmocka.h>

#include "common.h"
#include "mdns/mdns.h"
#include "test_mdns_avahi_harness.h"

extern mdns_backend mdns_avahi;

static int setup(__attribute__((unused)) void **state) {
  mdns_avahi_test_reset_stubs();
  memset(&config, 0, sizeof(config));
  config.regtype = "_raop._tcp";
  config.regtype2 = "_airplay._tcp";
  return 0;
}

static void test_update_empty_arrays_then_unregister_has_no_double_free(__attribute__((unused)) void **state) {
  char *primary_txt[] = {"txtvers=1", NULL};
  char *secondary_txt[] = {"sf=0x4", NULL};
  char *empty_txt[] = {NULL};

  assert_int_equal(mdns_avahi.mdns_register("010203040506@Receiver", "Receiver", 7000, primary_txt,
                                            secondary_txt),
                   0);
  assert_int_equal(mdns_avahi.mdns_update(empty_txt, empty_txt), 0);
  mdns_avahi.mdns_unregister();

  assert_int_equal(mdns_avahi_stub_string_list_double_free_detected, 0);
}

static void test_update_propagates_primary_error(__attribute__((unused)) void **state) {
  char *primary_txt[] = {"txtvers=1", NULL};
  char *secondary_txt[] = {"sf=0x4", NULL};
  char *updated_primary_txt[] = {"cn=0,1", NULL};

  assert_int_equal(mdns_avahi.mdns_register("010203040506@Receiver", "Receiver", 7000, primary_txt,
                                            secondary_txt),
                   0);

  mdns_avahi_stub_update_primary_result = -7;
  assert_int_equal(mdns_avahi.mdns_update(updated_primary_txt, NULL), -7);
  mdns_avahi.mdns_unregister();
}

static void test_unregister_frees_group_and_allows_reregister(__attribute__((unused)) void **state) {
  char *primary_txt[] = {"txtvers=1", NULL};
  char *secondary_txt[] = {"sf=0x4", NULL};

  assert_int_equal(mdns_avahi.mdns_register("010203040506@Receiver", "Receiver", 7000, primary_txt,
                                            secondary_txt),
                   0);
  mdns_avahi.mdns_unregister();
  assert_int_equal(mdns_avahi_stub_group_free_count, 1);

  assert_int_equal(mdns_avahi.mdns_register("010203040506@Receiver", "Receiver", 7000, primary_txt,
                                            secondary_txt),
                   0);
  mdns_avahi.mdns_unregister();

  assert_int_equal(mdns_avahi_stub_group_free_count, 2);
  assert_int_equal(mdns_avahi_stub_add_service_call_count, 4);
}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test_setup_teardown(
          test_update_empty_arrays_then_unregister_has_no_double_free, setup, NULL),
      cmocka_unit_test_setup_teardown(test_update_propagates_primary_error, setup, NULL),
      cmocka_unit_test_setup_teardown(test_unregister_frees_group_and_allows_reregister, setup,
                                      NULL),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
