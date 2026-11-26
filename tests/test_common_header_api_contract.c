#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h>
#include "common.h"

typedef const char *(*short_format_description_sig_t)(int32_t encoded_format);
typedef const char *(*sps_format_description_string_sig_t)(sps_format_t format);
typedef unsigned int (*sps_format_sample_size_sig_t)(sps_format_t format);
typedef unsigned int (*sps_rate_actual_rate_sig_t)(sps_rate_t rate);
typedef APST_t (*string_to_service_type_sig_t)(const char *parameter, const char *setting_name);
typedef void (*service_type_to_string_sig_t)(APST_t service_type, char *string_space);
typedef int64_t (*generate_zero_frames_sig_t)(char *outp, size_t number_of_frames, int with_dither,
                                              int64_t random_number_in,
                                              uint32_t encoded_output_format);
typedef int (*string_update_with_size_sig_t)(char **str, int *flag, char *s, size_t len);
typedef void *(*memdup_sig_t)(const void *mem, size_t size);

_Static_assert(
    __builtin_types_compatible_p(__typeof__(&short_format_description),
                                 short_format_description_sig_t),
    "short_format_description signature drifted from common.h contract");
_Static_assert(
    __builtin_types_compatible_p(__typeof__(&sps_format_description_string),
                                 sps_format_description_string_sig_t),
    "sps_format_description_string signature drifted from common.h contract");
_Static_assert(
    __builtin_types_compatible_p(__typeof__(&sps_format_sample_size), sps_format_sample_size_sig_t),
    "sps_format_sample_size signature drifted from common.h contract");
_Static_assert(
    __builtin_types_compatible_p(__typeof__(&sps_rate_actual_rate), sps_rate_actual_rate_sig_t),
    "sps_rate_actual_rate signature drifted from common.h contract");
_Static_assert(
    __builtin_types_compatible_p(__typeof__(&string_to_service_type), string_to_service_type_sig_t),
    "string_to_service_type signature drifted from common.h contract");
_Static_assert(
    __builtin_types_compatible_p(__typeof__(&service_type_to_string), service_type_to_string_sig_t),
    "service_type_to_string signature drifted from common.h contract");
_Static_assert(
    __builtin_types_compatible_p(__typeof__(&generate_zero_frames), generate_zero_frames_sig_t),
    "generate_zero_frames signature drifted from common.h contract");
_Static_assert(
    __builtin_types_compatible_p(__typeof__(&string_update_with_size), string_update_with_size_sig_t),
    "string_update_with_size signature drifted from common.h contract");
_Static_assert(__builtin_types_compatible_p(__typeof__(&memdup), memdup_sig_t),
               "memdup signature drifted from common.h contract");

static void test_common_header_api_contract_compiles(__attribute__((unused)) void **state) {}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test_common_header_api_contract_compiles),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
