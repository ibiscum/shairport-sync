#include "common.h"
#include "metadata/core.h"
#include "player.h"
#include <pthread.h>
#include <stdint.h>
#include <string.h>

shairport_cfg config = {.bitrate_stats_interval = 1.25};
static uint64_t test_now_ns = 0;
static unsigned test_send_ssnc_count = 0;
static uint32_t test_last_code = 0;
static char test_last_payload[128];

void test_audio_bitrate_stub_reset_metadata(void) {
  test_send_ssnc_count = 0;
  test_last_code = 0;
  test_last_payload[0] = '\0';
}

void test_audio_bitrate_stub_set_time_ns(uint64_t now_ns) { test_now_ns = now_ns; }

unsigned test_audio_bitrate_stub_send_count(void) { return test_send_ssnc_count; }

uint32_t test_audio_bitrate_stub_last_code(void) { return test_last_code; }

const char *test_audio_bitrate_stub_last_payload(void) { return test_last_payload; }

int send_ssnc_metadata(const uint32_t code, const char *data, const uint32_t length,
                       __attribute__((unused)) const int block) {
  test_send_ssnc_count++;
  test_last_code = code;
  if (data && length) {
    size_t copy_length = length < sizeof(test_last_payload) - 1 ? length : sizeof(test_last_payload) - 1;
    memcpy(test_last_payload, data, copy_length);
    test_last_payload[copy_length] = '\0';
  } else {
    test_last_payload[0] = '\0';
  }
  return 0;
}

uint32_t get_ssrc_rate(__attribute__((unused)) ssrc_t ssrc) { return 44100; }

uint64_t get_absolute_time_in_ns(void) { return test_now_ns; }

void mutex_unlock(void *arg) { pthread_mutex_unlock((pthread_mutex_t *)arg); }
