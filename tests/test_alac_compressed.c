#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <string.h>

#include <cmocka.h>

#include "alac.h"

typedef struct {
  uint8_t data[128];
  size_t bitpos;
} bit_writer;

static void bit_writer_put(bit_writer *w, uint32_t value, int bits) {
  for (int i = bits - 1; i >= 0; i--) {
    const uint8_t bit = (value >> i) & 1;
    const size_t byte_index = w->bitpos / 8;
    const size_t bit_index = 7 - (w->bitpos % 8);
    if (bit)
      w->data[byte_index] |= (uint8_t)(1U << bit_index);
    w->bitpos++;
  }
}

static alac_file *make_alac(int sample_size, int channels, int max_samples) {
  alac_file *alac = alac_create(sample_size, channels);
  assert_non_null(alac);

  alac->setinfo_max_samples_per_frame = (uint32_t)max_samples;
  alac->setinfo_sample_size = (uint8_t)sample_size;
  alac->setinfo_rice_initialhistory = 10;
  alac->setinfo_rice_kmodifier = 4;
  alac->setinfo_rice_historymult = 40;

  alac_allocate_buffers(alac);
  assert_non_null(alac->predicterror_buffer_a);
  assert_non_null(alac->outputsamples_buffer_a);
  return alac;
}

static void test_alac_decode_frame_mono_compressed_16bit(void **state) {
  (void)state;
  alac_file *alac = make_alac(16, 1, 1);

  bit_writer w = {0};
  bit_writer_put(&w, 0, 3); /* mono */
  bit_writer_put(&w, 0, 4);
  bit_writer_put(&w, 0, 12);
  bit_writer_put(&w, 1, 1); /* hassize */
  bit_writer_put(&w, 0, 2); /* uncompressed_bytes */
  bit_writer_put(&w, 0, 1); /* compressed */
  bit_writer_put(&w, 1, 32); /* outputsamples */
  bit_writer_put(&w, 0, 8);
  bit_writer_put(&w, 0, 8);
  bit_writer_put(&w, 0, 4); /* prediction_type */
  bit_writer_put(&w, 4, 4); /* prediction_quantitization */
  bit_writer_put(&w, 0, 3); /* ricemodifier */
  bit_writer_put(&w, 0, 5); /* predictor_coef_num */

  /* entropy: unary x=1 -> decodedValue=1 -> final sample = -1 */
  bit_writer_put(&w, 1, 1);
  bit_writer_put(&w, 0, 1);

  int16_t out = 0;
  int output_size = (int)sizeof(out);
  alac_decode_frame(alac, w.data, &out, &output_size);

  assert_int_equal(output_size, (int)sizeof(out));
  assert_int_equal(out, -1);

  alac_free(alac);
}

static void test_alac_decode_frame_stereo_compressed_weighted_16bit(void **state) {
  (void)state;
  alac_file *alac = make_alac(16, 2, 1);

  bit_writer w = {0};
  bit_writer_put(&w, 1, 3); /* stereo */
  bit_writer_put(&w, 0, 4);
  bit_writer_put(&w, 0, 12);
  bit_writer_put(&w, 1, 1); /* hassize */
  bit_writer_put(&w, 0, 2); /* uncompressed_bytes */
  bit_writer_put(&w, 0, 1); /* compressed */
  bit_writer_put(&w, 1, 32); /* outputsamples */
  bit_writer_put(&w, 0, 8); /* interlacing_shift */
  bit_writer_put(&w, 1, 8); /* interlacing_leftweight to hit weighted path */

  bit_writer_put(&w, 0, 4); /* prediction_type_a */
  bit_writer_put(&w, 4, 4); /* prediction_quantitization_a */
  bit_writer_put(&w, 0, 3); /* ricemodifier_a */
  bit_writer_put(&w, 0, 5); /* predictor_coef_num_a */

  bit_writer_put(&w, 0, 4); /* prediction_type_b */
  bit_writer_put(&w, 4, 4); /* prediction_quantitization_b */
  bit_writer_put(&w, 0, 3); /* ricemodifier_b */
  bit_writer_put(&w, 0, 5); /* predictor_coef_num_b */

  /* channel A entropy => sample -1 */
  bit_writer_put(&w, 1, 1);
  bit_writer_put(&w, 0, 1);
  /* channel B entropy => sample -1 */
  bit_writer_put(&w, 1, 1);
  bit_writer_put(&w, 0, 1);

  int16_t out[2] = {0, 0};
  int output_size = (int)sizeof(out);
  alac_decode_frame(alac, w.data, out, &output_size);

  assert_int_equal(output_size, (int)sizeof(out));
  assert_int_equal(out[0], -1);
  assert_int_equal(out[1], 0);

  alac_free(alac);
}

static void test_alac_decode_frame_mono_compressed_zero_block_and_raw_path(void **state) {
  (void)state;
  alac_file *alac = make_alac(16, 1, 2);

  bit_writer w = {0};
  bit_writer_put(&w, 0, 3); /* mono */
  bit_writer_put(&w, 0, 4);
  bit_writer_put(&w, 0, 12);
  bit_writer_put(&w, 1, 1); /* hassize */
  bit_writer_put(&w, 0, 2); /* uncompressed_bytes */
  bit_writer_put(&w, 0, 1); /* compressed */
  bit_writer_put(&w, 2, 32); /* outputsamples */
  bit_writer_put(&w, 0, 8);
  bit_writer_put(&w, 0, 8);
  bit_writer_put(&w, 0, 4); /* prediction_type */
  bit_writer_put(&w, 4, 4); /* prediction_quantitization */
  bit_writer_put(&w, 0, 3); /* ricemodifier */
  bit_writer_put(&w, 0, 5); /* predictor_coef_num */

  /* First entropy decode: unary x=1 => -1 */
  bit_writer_put(&w, 1, 1);
  bit_writer_put(&w, 0, 1);
  /* Zero-block special case: blockSize = 1 */
  bit_writer_put(&w, 0, 1);
  bit_writer_put(&w, 2, 3);

  int16_t out[2] = {0, 0};
  int output_size = (int)sizeof(out);
  alac_decode_frame(alac, w.data, out, &output_size);

  assert_int_equal(output_size, (int)sizeof(out));
  assert_int_equal(out[0], -1);
  assert_int_equal(out[1], 0);

  /* Second decode in same test to exercise raw value branch (x > RICE_THRESHOLD). */
  memset(&w, 0, sizeof(w));
  bit_writer_put(&w, 0, 3); /* mono */
  bit_writer_put(&w, 0, 4);
  bit_writer_put(&w, 0, 12);
  bit_writer_put(&w, 1, 1); /* hassize */
  bit_writer_put(&w, 0, 2); /* uncompressed_bytes */
  bit_writer_put(&w, 0, 1); /* compressed */
  bit_writer_put(&w, 1, 32); /* outputsamples */
  bit_writer_put(&w, 0, 8);
  bit_writer_put(&w, 0, 8);
  bit_writer_put(&w, 0, 4); /* prediction_type */
  bit_writer_put(&w, 4, 4); /* prediction_quantitization */
  bit_writer_put(&w, 0, 3); /* ricemodifier */
  bit_writer_put(&w, 0, 5); /* predictor_coef_num */

  /* Nine leading ones force raw-value path, then provide raw 16-bit value = 2 -> sample +1 */
  bit_writer_put(&w, 0x1FF, 9);
  bit_writer_put(&w, 2, 16);

  int16_t out2[2] = {0, 0};
  output_size = (int)sizeof(out2);
  alac_decode_frame(alac, w.data, out2, &output_size);

  assert_int_equal(output_size, (int)sizeof(int16_t));
  assert_int_equal(out2[0], 1);

  alac_free(alac);
}

static void test_alac_decode_frame_stereo_compressed_24bit_weighted_with_uncompressed_bytes(
    void **state) {
  (void)state;
  alac_file *alac = make_alac(24, 2, 1);

  bit_writer w = {0};
  bit_writer_put(&w, 1, 3); /* stereo */
  bit_writer_put(&w, 0, 4);
  bit_writer_put(&w, 0, 12);
  bit_writer_put(&w, 1, 1); /* hassize */
  bit_writer_put(&w, 1, 2); /* uncompressed_bytes = 1 */
  bit_writer_put(&w, 0, 1); /* compressed */
  bit_writer_put(&w, 1, 32); /* outputsamples */

  bit_writer_put(&w, 0, 8); /* interlacing_shift */
  bit_writer_put(&w, 1, 8); /* interlacing_leftweight */

  bit_writer_put(&w, 0, 4); /* prediction_type_a */
  bit_writer_put(&w, 4, 4); /* prediction_quantitization_a */
  bit_writer_put(&w, 0, 3); /* ricemodifier_a */
  bit_writer_put(&w, 0, 5); /* predictor_coef_num_a */

  bit_writer_put(&w, 0, 4); /* prediction_type_b */
  bit_writer_put(&w, 4, 4); /* prediction_quantitization_b */
  bit_writer_put(&w, 0, 3); /* ricemodifier_b */
  bit_writer_put(&w, 0, 5); /* predictor_coef_num_b */

  /* one sample's uncompressed low byte per channel */
  bit_writer_put(&w, 0x34, 8); /* channel A */
  bit_writer_put(&w, 0x12, 8); /* channel B */

  /* entropy for channel A: unary x=1 => sample -1 */
  bit_writer_put(&w, 1, 1);
  bit_writer_put(&w, 0, 1);
  /* entropy for channel B: unary x=1 => sample -1 */
  bit_writer_put(&w, 1, 1);
  bit_writer_put(&w, 0, 1);

  uint8_t out[6] = {0};
  int output_size = (int)sizeof(out);
  alac_decode_frame(alac, w.data, out, &output_size);

  assert_int_equal(output_size, (int)sizeof(out));
  /* left = 0xFFFF34, right = 0x000012 in little-endian 24-bit packed output */
  assert_int_equal(out[0], 0x34);
  assert_int_equal(out[1], 0xFF);
  assert_int_equal(out[2], 0xFF);
  assert_int_equal(out[3], 0x12);
  assert_int_equal(out[4], 0x00);
  assert_int_equal(out[5], 0x00);

  alac_free(alac);
}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test_alac_decode_frame_mono_compressed_16bit),
      cmocka_unit_test(test_alac_decode_frame_stereo_compressed_weighted_16bit),
      cmocka_unit_test(test_alac_decode_frame_mono_compressed_zero_block_and_raw_path),
      cmocka_unit_test(
          test_alac_decode_frame_stereo_compressed_24bit_weighted_with_uncompressed_bytes),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
