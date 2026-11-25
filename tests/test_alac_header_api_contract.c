#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h>
#include "alac.h"

typedef alac_file *(*alac_create_sig_t)(int samplesize, int numchannels);
typedef void (*alac_decode_frame_sig_t)(alac_file *alac, const unsigned char *inbuffer,
                                        void *outbuffer, int *outputsize);
typedef void (*alac_set_info_sig_t)(alac_file *alac, const char *inputbuffer);
typedef void (*alac_allocate_buffers_sig_t)(alac_file *alac);
typedef void (*alac_free_sig_t)(alac_file *alac);

_Static_assert(__builtin_types_compatible_p(__typeof__(&alac_create), alac_create_sig_t),
               "alac_create signature drifted from alac.h contract");
_Static_assert(__builtin_types_compatible_p(__typeof__(&alac_decode_frame), alac_decode_frame_sig_t),
               "alac_decode_frame signature drifted from alac.h contract");
_Static_assert(__builtin_types_compatible_p(__typeof__(&alac_set_info), alac_set_info_sig_t),
               "alac_set_info signature drifted from alac.h contract");
_Static_assert(
    __builtin_types_compatible_p(__typeof__(&alac_allocate_buffers), alac_allocate_buffers_sig_t),
    "alac_allocate_buffers signature drifted from alac.h contract");
_Static_assert(__builtin_types_compatible_p(__typeof__(&alac_free), alac_free_sig_t),
               "alac_free signature drifted from alac.h contract");

static void test_alac_header_api_contract_compiles(__attribute__((unused)) void **state) {}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test_alac_header_api_contract_compiles),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
