#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h>
#include "common.h"
#include "mdns/mdns.h"

shairport_cfg config;

#if defined(__GNUC__) || defined(__clang__)

typedef void (*mdns_unregister_sig_t)(void);
typedef void (*mdns_register_sig_t)(char **txt_records, char **secondary_txt_records);
typedef void (*mdns_update_sig_t)(char **txt_records, char **secondary_txt_records);
typedef void (*mdns_dacp_monitor_start_sig_t)(void);
typedef void (*mdns_dacp_monitor_stop_sig_t)(void);
typedef void (*mdns_dacp_monitor_set_id_sig_t)(const char *dacp_id);

_Static_assert(__builtin_types_compatible_p(__typeof__(&mdns_unregister), mdns_unregister_sig_t),
               "mdns_unregister signature drifted from mdns.h contract");
_Static_assert(__builtin_types_compatible_p(__typeof__(&mdns_register), mdns_register_sig_t),
               "mdns_register signature drifted from mdns.h contract");
_Static_assert(__builtin_types_compatible_p(__typeof__(&mdns_update), mdns_update_sig_t),
               "mdns_update signature drifted from mdns.h contract");
_Static_assert(
    __builtin_types_compatible_p(__typeof__(&mdns_dacp_monitor_start),
                                 mdns_dacp_monitor_start_sig_t),
    "mdns_dacp_monitor_start signature drifted from mdns.h contract");
_Static_assert(
    __builtin_types_compatible_p(__typeof__(&mdns_dacp_monitor_stop), mdns_dacp_monitor_stop_sig_t),
    "mdns_dacp_monitor_stop signature drifted from mdns.h contract");
_Static_assert(
    __builtin_types_compatible_p(__typeof__(&mdns_dacp_monitor_set_id),
                                 mdns_dacp_monitor_set_id_sig_t),
    "mdns_dacp_monitor_set_id signature drifted from mdns.h contract");

#endif

static void test_mdns_header_api_contract_compiles(__attribute__((unused)) void **state) {
  const char *without_metadata[] = {MDNS_RECORD_WITHOUT_METADATA, NULL};
  (void)without_metadata;
#ifdef CONFIG_METADATA
  const char *with_metadata[] = {MDNS_RECORD_WITH_METADATA, NULL};
  (void)with_metadata;
#endif
}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test_mdns_header_api_contract_compiles),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
