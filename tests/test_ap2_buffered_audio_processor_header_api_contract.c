#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h>
#include "ap2_buffered_audio_processor.h"

#if defined(__GNUC__) || defined(__clang__)
typedef void *(*rtp_buffered_audio_processor_sig_t)(void *arg);

_Static_assert(
    __builtin_types_compatible_p(__typeof__(&rtp_buffered_audio_processor),
                                 rtp_buffered_audio_processor_sig_t),
    "rtp_buffered_audio_processor signature drifted from ap2_buffered_audio_processor.h contract");
#endif

static void test_ap2_buffered_audio_processor_header_api_contract_compiles(
    __attribute__((unused)) void **state) {}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test_ap2_buffered_audio_processor_header_api_contract_compiles),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
