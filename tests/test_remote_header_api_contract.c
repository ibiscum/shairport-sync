#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h>
#include "remote/remote.h"

#if defined(__GNUC__) || defined(__clang__)

typedef int (*remote_set_airplay_volume_sig_t)(double volume);
typedef int (*remote_set_integer_percent_volume_sig_t)(const int volume);
typedef int (*remote_set_repeat_mode_sig_t)(repeat_status_type mode);
typedef int (*remote_set_shuffle_mode_sig_t)(shuffle_status_type mode);
typedef void (*remote_simple_command_sig_t)(simple_command_t command);
typedef ssize_t (*ap2_event_send_dev_mule_sig_t)(unsigned int command_number);

_Static_assert(__builtin_types_compatible_p(__typeof__(&remote_set_airplay_volume),
                                            remote_set_airplay_volume_sig_t),
               "remote_set_airplay_volume signature drifted from remote.h contract");
_Static_assert(__builtin_types_compatible_p(__typeof__(&remote_set_integer_percent_volume),
                                            remote_set_integer_percent_volume_sig_t),
               "remote_set_integer_percent_volume signature drifted from remote.h contract");
_Static_assert(__builtin_types_compatible_p(__typeof__(&remote_set_repeat_mode),
                                            remote_set_repeat_mode_sig_t),
               "remote_set_repeat_mode signature drifted from remote.h contract");
_Static_assert(__builtin_types_compatible_p(__typeof__(&remote_set_shuffle_mode),
                                            remote_set_shuffle_mode_sig_t),
               "remote_set_shuffle_mode signature drifted from remote.h contract");
_Static_assert(__builtin_types_compatible_p(__typeof__(&remote_simple_command),
                                            remote_simple_command_sig_t),
               "remote_simple_command signature drifted from remote.h contract");
_Static_assert(__builtin_types_compatible_p(__typeof__(&ap2_event_send_dev_mule),
                                            ap2_event_send_dev_mule_sig_t),
               "ap2_event_send_dev_mule signature drifted from remote.h contract");

_Static_assert(rcsc_play == 0, "rcsc_play value drifted");
_Static_assert(rcsc_pause == 1, "rcsc_pause value drifted");
_Static_assert(rcsc_play_pause == 2, "rcsc_play_pause value drifted");
_Static_assert(rcsc_stop == 3, "rcsc_stop value drifted");
_Static_assert(rcsc_next_item == 4, "rcsc_next_item value drifted");
_Static_assert(rcsc_previous_item == 5, "rcsc_previous_item value drifted");
_Static_assert(rcsc_toggle_shuffle == 6, "rcsc_toggle_shuffle value drifted");
_Static_assert(rcsc_cycle_repeat == 7, "rcsc_cycle_repeat value drifted");
_Static_assert(rcsc_fast_forward == 8, "rcsc_fast_forward value drifted");
_Static_assert(rcsc_fast_forward_stop == 9, "rcsc_fast_forward_stop value drifted");
_Static_assert(rcsc_rewind == 10, "rcsc_rewind value drifted");
_Static_assert(rcsc_rewind_stop == 11, "rcsc_rewind_stop value drifted");
_Static_assert(rcsc_volume_up == 12, "rcsc_volume_up value drifted");
_Static_assert(rcsc_volume_down == 13, "rcsc_volume_down value drifted");
_Static_assert(rcsc_not_a_command == 14, "rcsc_not_a_command value drifted");
_Static_assert(rcsc_command == 15, "rcsc_command value drifted");
_Static_assert(rcsc_disconnect == 16, "rcsc_disconnect value drifted");

#endif

static void test_remote_header_api_contract_compiles(__attribute__((unused)) void **state) {}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test_remote_header_api_contract_compiles),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
