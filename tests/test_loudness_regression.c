#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <cmocka.h>

#include "common.h"
#include "loudness.h"

extern void _loudness_set_volume(loudness_processor_static *p, float volume, unsigned int sample_rate);
extern loudness_processor_dynamic loudness_filters[];

static volume_range_t external_volume_range = {.minimum_volume_dB = -6000, .maximum_volume_dB = 0};
static output_parameters_t external_volume_parameters = {.volume_range = &external_volume_range};

static output_parameters_t *return_external_volume_parameters(void) {
  return &external_volume_parameters;
}

static int setup(void **state) {
  (void)state;
  memset(&config, 0, sizeof(config));
  config.loudness_enabled = 1;
  config.loudness_reference_volume_db = -16.0f;
  loudness_reset();
  return 0;
}

static void test_set_volume_bypasses_filter_for_too_low_sample_rate(void **state) {
  (void)state;
  loudness_processor_static coeffs = {99.0f, 98.0f, 97.0f, 96.0f, 95.0f};

  _loudness_set_volume(&coeffs, -20.0f, 20);

  assert_float_equal(coeffs.a0, 1.0f, 0.000001f);
  assert_float_equal(coeffs.a1, 0.0f, 0.000001f);
  assert_float_equal(coeffs.a2, 0.0f, 0.000001f);
  assert_float_equal(coeffs.b1, 0.0f, 0.000001f);
  assert_float_equal(coeffs.b2, 0.0f, 0.000001f);
}

static void test_set_volume_clamps_negative_gain_to_identity_transfer(void **state) {
  (void)state;
  loudness_processor_static coeffs = {0};

  _loudness_set_volume(&coeffs, 0.0f, 44100);

  assert_float_equal(coeffs.a0, 1.0f, 0.0001f);
  assert_float_equal(coeffs.a1, coeffs.b1, 0.0001f);
  assert_float_equal(coeffs.a2, coeffs.b2, 0.0001f);
}

static void test_loudness_update_disables_with_external_volume_controls(void **state) {
  (void)state;
  rtsp_conn_info conn;
  memset(&conn, 0, sizeof(conn));
  conn.fix_volume = 65536;
  conn.input_rate = 44100;

  static audio_output fake_output = {.parameters = return_external_volume_parameters};
  config.output = &fake_output;

  loudness_update(&conn);

  assert_int_equal(conn.do_loudness, 0);
}

static void test_loudness_process_blocks_clamps_to_filter_bank_size(void **state) {
  (void)state;
  rtsp_conn_info conn;
  memset(&conn, 0, sizeof(conn));
  conn.fix_volume = 1;
  conn.input_rate = 48000;
  config.output = NULL;
  loudness_update(&conn);

  conn.fix_volume = 65536;
  conn.input_rate = 44100;
  loudness_update(&conn);
  assert_int_equal(conn.do_loudness, 1);

  float fbufs[10] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f};
  loudness_process_blocks(fbufs, 1, 10, 2.0f);

  for (size_t i = 0; i < 10; i++) {
    assert_float_equal(fbufs[i], (float)(2 * (int)(i + 1)), 0.001f);
  }
}

static void test_loudness_reset_clears_filter_state(void **state) {
  (void)state;
  loudness_filters[0].i1 = 1.0f;
  loudness_filters[0].i2 = 2.0f;
  loudness_filters[0].o1 = 3.0f;
  loudness_filters[0].o2 = 4.0f;
  loudness_filters[7].i1 = 11.0f;
  loudness_filters[7].i2 = 12.0f;
  loudness_filters[7].o1 = 13.0f;
  loudness_filters[7].o2 = 14.0f;

  loudness_reset();

  assert_float_equal(loudness_filters[0].i1, 0.0f, 0.0f);
  assert_float_equal(loudness_filters[0].i2, 0.0f, 0.0f);
  assert_float_equal(loudness_filters[0].o1, 0.0f, 0.0f);
  assert_float_equal(loudness_filters[0].o2, 0.0f, 0.0f);
  assert_float_equal(loudness_filters[7].i1, 0.0f, 0.0f);
  assert_float_equal(loudness_filters[7].i2, 0.0f, 0.0f);
  assert_float_equal(loudness_filters[7].o1, 0.0f, 0.0f);
  assert_float_equal(loudness_filters[7].o2, 0.0f, 0.0f);
}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test_setup(test_set_volume_bypasses_filter_for_too_low_sample_rate, setup),
      cmocka_unit_test_setup(test_set_volume_clamps_negative_gain_to_identity_transfer, setup),
      cmocka_unit_test_setup(test_loudness_update_disables_with_external_volume_controls, setup),
      cmocka_unit_test_setup(test_loudness_process_blocks_clamps_to_filter_bank_size, setup),
      cmocka_unit_test_setup(test_loudness_reset_clears_filter_state, setup),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
