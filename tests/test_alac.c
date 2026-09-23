#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <string.h>

#include <cmocka.h>

#include "alac.h"

typedef struct {
  uint8_t data[64];
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

static void write_be16(uint8_t *p, uint16_t v) {
  p[0] = (uint8_t)((v >> 8) & 0xFF);
  p[1] = (uint8_t)(v & 0xFF);
}

static void write_be32(uint8_t *p, uint32_t v) {
  p[0] = (uint8_t)((v >> 24) & 0xFF);
  p[1] = (uint8_t)((v >> 16) & 0xFF);
  p[2] = (uint8_t)((v >> 8) & 0xFF);
  p[3] = (uint8_t)(v & 0xFF);
}

static void test_alac_create_initializes_fields(void **state) {
  (void)state;
  alac_file *alac = alac_create(16, 2);

  assert_non_null(alac);
  assert_int_equal(alac->samplesize, 16);
  assert_int_equal(alac->numchannels, 2);
  assert_int_equal(alac->bytespersample, 4);

  assert_null(alac->predicterror_buffer_a);
  assert_null(alac->predicterror_buffer_b);
  assert_null(alac->outputsamples_buffer_a);
  assert_null(alac->outputsamples_buffer_b);
  assert_null(alac->uncompressed_bytes_buffer_a);
  assert_null(alac->uncompressed_bytes_buffer_b);

  alac_free(alac);
}

static void test_alac_set_info_parses_values_and_allocates_buffers(void **state) {
  (void)state;
  uint8_t atom[48];
  memset(atom, 0, sizeof(atom));

  uint8_t *p = atom + 24;
  write_be32(p, 2);
  p += 4;
  *p++ = 0x11;
  *p++ = 0x10;
  *p++ = 0x22;
  *p++ = 0x33;
  *p++ = 0x04;
  *p++ = 0x55;
  write_be16(p, 0x0A0B);
  p += 2;
  write_be32(p, 0x01020304);
  p += 4;
  write_be32(p, 0x05060708);
  p += 4;
  write_be32(p, 0x11223344);

  alac_file *alac = alac_create(16, 2);
  assert_non_null(alac);

  alac_set_info(alac, (char *)atom);

  assert_int_equal(alac->setinfo_max_samples_per_frame, 2);
  assert_int_equal(alac->setinfo_7a, 0x11);
  assert_int_equal(alac->setinfo_sample_size, 0x10);
  assert_int_equal(alac->setinfo_rice_historymult, 0x22);
  assert_int_equal(alac->setinfo_rice_initialhistory, 0x33);
  assert_int_equal(alac->setinfo_rice_kmodifier, 0x04);
  assert_int_equal(alac->setinfo_7f, 0x55);
  assert_int_equal(alac->setinfo_80, 0x0A0B);
  assert_int_equal(alac->setinfo_82, 0x01020304);
  assert_int_equal(alac->setinfo_86, 0x05060708);
  assert_int_equal(alac->setinfo_8a_rate, 0x11223344);

  assert_non_null(alac->predicterror_buffer_a);
  assert_non_null(alac->predicterror_buffer_b);
  assert_non_null(alac->outputsamples_buffer_a);
  assert_non_null(alac->outputsamples_buffer_b);
  assert_non_null(alac->uncompressed_bytes_buffer_a);
  assert_non_null(alac->uncompressed_bytes_buffer_b);

  alac_free(alac);
}

static void test_alac_decode_frame_mono_uncompressed_16bit(void **state) {
  (void)state;
  alac_file *alac = alac_create(16, 1);
  assert_non_null(alac);

  alac->setinfo_max_samples_per_frame = 1;
  alac->setinfo_sample_size = 16;
  alac_allocate_buffers(alac);

  bit_writer w = {0};
  bit_writer_put(&w, 0, 3);
  bit_writer_put(&w, 0, 4);
  bit_writer_put(&w, 0, 12);
  bit_writer_put(&w, 1, 1);
  bit_writer_put(&w, 0, 2);
  bit_writer_put(&w, 1, 1);
  bit_writer_put(&w, 1, 32);
  bit_writer_put(&w, 0x7FFF, 16);

  int16_t out = 0;
  int output_size = (int)sizeof(out);
  alac_decode_frame(alac, w.data, &out, &output_size);

  assert_int_equal(output_size, (int)sizeof(int16_t));
  assert_int_equal(out, 0x7FFF);

  alac_free(alac);
}

static void test_alac_decode_frame_stereo_uncompressed_16bit(void **state) {
  (void)state;
  alac_file *alac = alac_create(16, 2);
  assert_non_null(alac);

  alac->setinfo_max_samples_per_frame = 1;
  alac->setinfo_sample_size = 16;
  alac_allocate_buffers(alac);

  bit_writer w = {0};
  bit_writer_put(&w, 1, 3);
  bit_writer_put(&w, 0, 4);
  bit_writer_put(&w, 0, 12);
  bit_writer_put(&w, 1, 1);
  bit_writer_put(&w, 0, 2);
  bit_writer_put(&w, 1, 1);
  bit_writer_put(&w, 1, 32);
  bit_writer_put(&w, 0x1111, 16);
  bit_writer_put(&w, 0x2222, 16);

  int16_t out[2] = {0, 0};
  int output_size = (int)sizeof(out);
  alac_decode_frame(alac, w.data, out, &output_size);

  assert_int_equal(output_size, (int)sizeof(out));
  assert_int_equal(out[0], 0x1111);
  assert_int_equal(out[1], 0x2222);

  alac_free(alac);
}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test_alac_create_initializes_fields),
      cmocka_unit_test(test_alac_set_info_parses_values_and_allocates_buffers),
      cmocka_unit_test(test_alac_decode_frame_mono_uncompressed_16bit),
      cmocka_unit_test(test_alac_decode_frame_stereo_uncompressed_16bit),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
