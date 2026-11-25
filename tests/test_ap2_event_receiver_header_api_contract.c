#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h>
#include "ap2_event_receiver.h"

#if defined(__GNUC__) || defined(__clang__)
typedef ssize_t (*ap2_event_port_send_message_sig_t)(rtsp_conn_info *conn, char *data,
                                                     size_t data_length);
#ifdef CONFIG_AIRPLAY_2
typedef ssize_t (*ap2_event_port_post_command_sig_t)(rtsp_conn_info *conn, plist_t command);
#else
typedef ssize_t (*ap2_event_port_post_command_sig_t)(rtsp_conn_info *conn, const void *command);
#endif
typedef void *(*ap2_event_receiver_sig_t)(void *arg);

_Static_assert(
    __builtin_types_compatible_p(__typeof__(&ap2_event_port_send_message),
                                 ap2_event_port_send_message_sig_t),
    "ap2_event_port_send_message signature drifted from ap2_event_receiver.h contract");
_Static_assert(
    __builtin_types_compatible_p(__typeof__(&ap2_event_port_post_command),
                                 ap2_event_port_post_command_sig_t),
    "ap2_event_port_post_command signature drifted from ap2_event_receiver.h contract");
_Static_assert(__builtin_types_compatible_p(__typeof__(&ap2_event_receiver),
                                            ap2_event_receiver_sig_t),
               "ap2_event_receiver signature drifted from ap2_event_receiver.h contract");
#endif

static void test_ap2_event_receiver_header_api_contract_compiles(
    __attribute__((unused)) void **state) {}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test_ap2_event_receiver_header_api_contract_compiles),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
