#define _POSIX_C_SOURCE 200809L
#undef NDEBUG

#include <assert.h>
#include <arpa/inet.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "metadata/core.h"
#include "metadata/hub.h"
#include "metadata/pc_queue.h"

#ifndef CONFIG_METADATA_HUB
static void test_metadata_hub_disabled_build(__attribute__((unused)) void **state) {}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test_metadata_hub_disabled_build),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
#else

shairport_cfg config;

static int stub_send_metadata_to_queue_rc;
static unsigned stub_send_metadata_to_queue_calls;
static pc_queue *stub_last_queue;
static uint32_t stub_last_type;
static uint32_t stub_last_code;
static const char *stub_last_data;
static uint32_t stub_last_length;
static rtsp_message *stub_last_carrier;
static int stub_last_block;

#undef CONFIG_OPENSSL
#undef CONFIG_MBEDTLS
#undef CONFIG_POLARSSL
#undef CONFIG_AIRPLAY_2
#ifndef DT_REG
#define DT_REG 8
#endif
#include "../metadata/hub.c"

static void encode_u64_be(uint64_t value, unsigned char out[8]) {
  uint32_t hi = htonl((uint32_t)(value >> 32));
  uint32_t lo = htonl((uint32_t)(value & 0xffffffffu));
  memcpy(out, &hi, sizeof(hi));
  memcpy(out + sizeof(hi), &lo, sizeof(lo));
}

static void reset_state(void) {
  memset(&metadata_store, 0, sizeof(metadata_store));
  stub_send_metadata_to_queue_rc = 0;
  stub_send_metadata_to_queue_calls = 0;
  stub_last_queue = NULL;
  stub_last_type = 0;
  stub_last_code = 0;
  stub_last_data = NULL;
  stub_last_length = 0;
  stub_last_carrier = NULL;
  stub_last_block = -1;
  config.airplay_volume = -20.0;
  config.cover_art_cache_dir = "";
  config.retain_coverart = 0;
  metadata_hub_init();
}

static void test_assc_updates_sort_composer_only(__attribute__((unused)) void **state) {
  char composer[] = "Composer";
  char sort_composer[] = "Sort Composer";

  reset_state();
  metadata_hub_process_metadata(METADATA_FOURCC('c', 'o', 'r', 'e'),
                                METADATA_FOURCC('a', 's', 'c', 'p'), composer,
                                (uint32_t)strlen(composer));
  metadata_hub_process_metadata(METADATA_FOURCC('c', 'o', 'r', 'e'),
                                METADATA_FOURCC('a', 's', 's', 'c'), sort_composer,
                                (uint32_t)strlen(sort_composer));

  assert(metadata_store.npi.composer != NULL);
  assert(strcmp(metadata_store.npi.composer, "Composer") == 0);
  assert(metadata_store.npi.sort_composer != NULL);
  assert(strcmp(metadata_store.npi.sort_composer, "Sort Composer") == 0);
}

static void test_short_core_payloads_do_not_mutate_state(__attribute__((unused)) void **state) {
  char three_bytes[] = {0x00, 0x01, 0x02};
  char one_byte[] = {0x55};

  reset_state();
  metadata_store.npi.playing_state = 9;
  metadata_store.npi.songtime_in_microseconds.valid = 1;
  metadata_store.npi.songtime_in_microseconds.value = 1234;
  metadata_store.npi.track_number.valid = 1;
  metadata_store.npi.track_number.value = 7;

  metadata_hub_process_metadata(METADATA_FOURCC('c', 'o', 'r', 'e'),
                                METADATA_FOURCC('c', 'a', 'p', 's'), NULL, 0);
  metadata_hub_process_metadata(METADATA_FOURCC('c', 'o', 'r', 'e'),
                                METADATA_FOURCC('a', 's', 't', 'm'), three_bytes,
                                (uint32_t)sizeof(three_bytes));
  metadata_hub_process_metadata(METADATA_FOURCC('c', 'o', 'r', 'e'),
                                METADATA_FOURCC('a', 's', 't', 'n'), one_byte,
                                (uint32_t)sizeof(one_byte));

  assert(metadata_store.npi.playing_state == 9);
  assert(metadata_store.npi.songtime_in_microseconds.valid == 1);
  assert(metadata_store.npi.songtime_in_microseconds.value == 1234);
  assert(metadata_store.npi.track_number.valid == 1);
  assert(metadata_store.npi.track_number.value == 7);
}

static void test_picture_write_failure_does_not_store_null_uri(__attribute__((unused)) void **state) {
  char fake_image[20];
  memset(fake_image, 0x11, sizeof(fake_image));

  reset_state();
  config.cover_art_cache_dir = "/this/path/does/not/exist/and/should/fail";
  assert(metadata_hub_process_picture(fake_image, sizeof(fake_image)) == 0);
  assert(metadata_store.npi.cover_art_pathname == NULL);
}

static void test_mdst_mden_new_track_replaces_npi(__attribute__((unused)) void **state) {
  unsigned char id1[8];
  unsigned char id2[8];
  char old_title[] = "Old";
  char new_title[] = "New";

  reset_state();
  encode_u64_be(1, id1);
  encode_u64_be(2, id2);
  metadata_hub_process_metadata(METADATA_FOURCC('c', 'o', 'r', 'e'),
                                METADATA_FOURCC('m', 'i', 'n', 'm'), old_title,
                                (uint32_t)strlen(old_title));
  metadata_hub_process_metadata(METADATA_FOURCC('c', 'o', 'r', 'e'),
                                METADATA_FOURCC('m', 'p', 'e', 'r'), (char *)id1, sizeof(id1));

  metadata_hub_process_metadata(METADATA_FOURCC('s', 's', 'n', 'c'),
                                METADATA_FOURCC('m', 'd', 's', 't'), NULL, 0);
  metadata_hub_process_metadata(METADATA_FOURCC('c', 'o', 'r', 'e'),
                                METADATA_FOURCC('m', 'i', 'n', 'm'), new_title,
                                (uint32_t)strlen(new_title));
  metadata_hub_process_metadata(METADATA_FOURCC('c', 'o', 'r', 'e'),
                                METADATA_FOURCC('m', 'p', 'e', 'r'), (char *)id2, sizeof(id2));
  metadata_hub_process_metadata(METADATA_FOURCC('s', 's', 'n', 'c'),
                                METADATA_FOURCC('m', 'd', 'e', 'n'), NULL, 0);

  assert(metadata_store.npi.item_id.valid == 1);
  assert(metadata_store.npi.item_id.value == 2);
  assert(metadata_store.npi.track_name != NULL);
  assert(strcmp(metadata_store.npi.track_name, "New") == 0);
}

static void test_send_metadata_to_hub_queue_forwards_parameters(
    __attribute__((unused)) void **state) {
  rtsp_message carrier = {0};

  reset_state();
  stub_send_metadata_to_queue_rc = EWOULDBLOCK;
  assert(send_metadata_to_hub_queue(METADATA_FOURCC('s', 's', 'n', 'c'),
                                    METADATA_FOURCC('a', 'r', 's', 't'), "x", 1, &carrier, 1) ==
         EWOULDBLOCK);
  assert(stub_send_metadata_to_queue_calls == 1);
  assert(stub_last_queue != NULL);
  assert(stub_last_type == METADATA_FOURCC('s', 's', 'n', 'c'));
  assert(stub_last_code == METADATA_FOURCC('a', 'r', 's', 't'));
  assert(stub_last_data != NULL && stub_last_data[0] == 'x');
  assert(stub_last_length == 1);
  assert(stub_last_carrier == &carrier);
  assert(stub_last_block == 1);
}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test_assc_updates_sort_composer_only),
      cmocka_unit_test(test_short_core_payloads_do_not_mutate_state),
      cmocka_unit_test(test_picture_write_failure_does_not_store_null_uri),
      cmocka_unit_test(test_mdst_mden_new_track_replaces_npi),
      cmocka_unit_test(test_send_metadata_to_hub_queue_forwards_parameters),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}

int parse_prlg(__attribute__((unused)) const char *str, __attribute__((unused)) uint32_t *a,
               __attribute__((unused)) uint32_t *b, __attribute__((unused)) uint32_t *c) {
  return -1;
}

int mkpath(__attribute__((unused)) const char *path, __attribute__((unused)) mode_t mode) {
  errno = EACCES;
  return -1;
}

int named_pthread_create(__attribute__((unused)) pthread_t *restrict thread,
                         __attribute__((unused)) const pthread_attr_t *restrict attr,
                         __attribute__((unused)) void *(*start_routine)(void *),
                         __attribute__((unused)) void *restrict arg,
                         __attribute__((unused)) const char *format, ...) {
  return 0;
}

void pc_queue_init(__attribute__((unused)) pc_queue *the_queue, __attribute__((unused)) char *items,
                   __attribute__((unused)) size_t item_size,
                   __attribute__((unused)) uint32_t number_of_items,
                   __attribute__((unused)) const char *name) {}

void pc_queue_delete(__attribute__((unused)) pc_queue *the_queue) {}

int pc_queue_add_item(__attribute__((unused)) pc_queue *the_queue,
                      __attribute__((unused)) const void *the_stuff,
                      __attribute__((unused)) int block) {
  return 0;
}

int pc_queue_get_item(__attribute__((unused)) pc_queue *the_queue,
                      __attribute__((unused)) void *the_stuff) {
  return 0;
}

void metadata_pack_cleanup_function(__attribute__((unused)) void *arg) {}
void metadata_queue_delete(__attribute__((unused)) pc_queue *queue) {}

int send_metadata_to_queue(pc_queue *queue, const uint32_t type, const uint32_t code,
                           const char *data, const uint32_t length, rtsp_message *carrier,
                           int block) {
  stub_send_metadata_to_queue_calls++;
  stub_last_queue = queue;
  stub_last_type = type;
  stub_last_code = code;
  stub_last_data = data;
  stub_last_length = length;
  stub_last_carrier = carrier;
  stub_last_block = block;
  return stub_send_metadata_to_queue_rc;
}

void _debug(__attribute__((unused)) const char *filename, __attribute__((unused)) const int linenumber,
            __attribute__((unused)) int level, __attribute__((unused)) const char *format, ...) {}

void _warn(__attribute__((unused)) const char *filename, __attribute__((unused)) const int linenumber,
           __attribute__((unused)) const char *format, ...) {}

void _die(__attribute__((unused)) const char *filename, __attribute__((unused)) const int linenumber,
          __attribute__((unused)) const char *format, ...) {
  abort();
}

#endif
