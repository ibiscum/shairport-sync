#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h>
#include "loudness.h"

#if defined(__GNUC__) || defined(__clang__)
typedef float (*loudness_process_sig_t)(loudness_processor_dynamic *p, float sample);
typedef void (*loudness_update_sig_t)(rtsp_conn_info *conn);
typedef void (*loudness_reset_sig_t)(void);
typedef void (*loudness_process_blocks_sig_t)(float *fbufs, unsigned int channel_length,
                                              unsigned int number_of_channels, float gain);

_Static_assert(__builtin_types_compatible_p(__typeof__(&loudness_process), loudness_process_sig_t),
               "loudness_process signature drifted from loudness.h contract");
_Static_assert(__builtin_types_compatible_p(__typeof__(&loudness_update), loudness_update_sig_t),
               "loudness_update signature drifted from loudness.h contract");
_Static_assert(__builtin_types_compatible_p(__typeof__(&loudness_reset), loudness_reset_sig_t),
               "loudness_reset signature drifted from loudness.h contract");
_Static_assert(__builtin_types_compatible_p(__typeof__(&loudness_process_blocks),
                                            loudness_process_blocks_sig_t),
               "loudness_process_blocks signature drifted from loudness.h contract");
#endif

static void test_loudness_header_api_contract_compiles(__attribute__((unused)) void **state) {}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test_loudness_header_api_contract_compiles),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
