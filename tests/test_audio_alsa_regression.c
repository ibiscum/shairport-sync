#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <errno.h>

#include <cmocka.h>

#include "utilities/audio_alsa_safety.h"

static void test_validate_positive_int_rejects_non_positive_values(void **state) {
  (void)state;
  assert_int_equal(audio_alsa_validate_strictly_positive_int(-1), 0);
  assert_int_equal(audio_alsa_validate_strictly_positive_int(0), 0);
  assert_int_equal(audio_alsa_validate_strictly_positive_int(1), 1);
}

static void test_validate_play_request_rejects_invalid_combinations(void **state) {
  (void)state;
  assert_int_equal(audio_alsa_validate_play_request(NULL, 1), -EINVAL);
  assert_int_equal(audio_alsa_validate_play_request(NULL, -1), -EINVAL);
}

static void test_validate_play_request_accepts_valid_combinations(void **state) {
  (void)state;
  int sample = 0;
  assert_int_equal(audio_alsa_validate_play_request(NULL, 0), 0);
  assert_int_equal(audio_alsa_validate_play_request(&sample, 1), 0);
}

static void test_reset_channel_map_clears_previous_content(void **state) {
  (void)state;
  char map[16] = "FL FR";
  audio_alsa_reset_channel_map_string(map, sizeof(map));
  assert_string_equal(map, "");
}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test_validate_positive_int_rejects_non_positive_values),
      cmocka_unit_test(test_validate_play_request_rejects_invalid_combinations),
      cmocka_unit_test(test_validate_play_request_accepts_valid_combinations),
      cmocka_unit_test(test_reset_channel_map_clears_previous_content),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
