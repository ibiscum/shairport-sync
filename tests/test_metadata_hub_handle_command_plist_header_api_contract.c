#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h>
#include "metadata/metadata_hub_handle_command_plist.h"

#if defined(__GNUC__) || defined(__clang__)
#ifdef CONFIG_AIRPLAY_2
typedef void (*metadata_hub_handle_command_plist_sig_t)(rtsp_conn_info *conn,
                                                        const plist_t command_dict);
#else
typedef void (*metadata_hub_handle_command_plist_sig_t)(rtsp_conn_info *conn,
                                                        const void *command_dict);
#endif

_Static_assert(
    __builtin_types_compatible_p(__typeof__(&metadata_hub_handle_command_plist),
                                 metadata_hub_handle_command_plist_sig_t),
    "metadata_hub_handle_command_plist signature drifted from metadata_hub_handle_command_plist.h contract");
#endif

static void test_metadata_hub_handle_command_plist_header_api_contract_compiles(
    __attribute__((unused)) void **state) {}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test_metadata_hub_handle_command_plist_header_api_contract_compiles),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
