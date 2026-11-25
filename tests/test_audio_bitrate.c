/* Keep meter operations and checks active with release build flags. */
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "metadata/audio_bitrate.h"
#include "metadata/core.h"

void test_audio_bitrate_stub_reset_metadata(void);
void test_audio_bitrate_stub_set_time_ns(uint64_t now_ns);
unsigned test_audio_bitrate_stub_send_count(void);
uint32_t test_audio_bitrate_stub_last_code(void);
const char *test_audio_bitrate_stub_last_payload(void);

static void init_conn_mutexes(rtsp_conn_info *conn) {
  assert(!pthread_mutex_init(&conn->ab_mutex, NULL));
  assert(!pthread_mutex_init(&conn->flush_mutex, NULL));
}

static void destroy_conn_mutexes(rtsp_conn_info *conn) {
  assert(!pthread_mutex_destroy(&conn->ab_mutex));
  assert(!pthread_mutex_destroy(&conn->flush_mutex));
}

int main(void) {
  audio_bitrate_meter meter = {0};
  unsigned reports = 0;
  for (unsigned i = 0; i < 141; i++)
    reports += audio_bitrate_add(&meter, 680 + i % 3, 1024, 48000, i, i * 1024);
  assert(reports == 2);
  assert(meter.samples == 141 * 1024 && meter.bytes == 141 * 681);
  uint64_t bytes = meter.bytes, samples = meter.samples;
  assert(!audio_bitrate_add(&meter, 1000, 1024, 48000, 140, 140 * 1024));
  assert(meter.bytes == bytes && meter.samples == samples);
  assert(!audio_bitrate_add(&meter, 1000, 0, 48000, 141, 141 * 1024));
  assert(!audio_bitrate_add(&meter, 1000, 1024, 0, 141, 141 * 1024));
  assert(meter.bytes == bytes && meter.samples == samples);
  assert(audio_bitrate_reset(&meter) && !audio_bitrate_reset(&meter));
  assert(audio_bitrate_add(&meter, 64000, 96000, 48000, 0, 0));
  assert(meter.bytes * 8 * meter.rate / meter.samples == 256000);
  assert(audio_bitrate_add(&meter, 64000, 88200, 44100, 1, 0));
  assert(meter.bytes == 64000 && meter.samples == 88200);
  assert(meter.bytes * 8 * meter.rate / meter.samples == 256000);
  audio_bitrate_reset(&meter);
  assert(audio_bitrate_add(&meter, 32000, 48000, 48000, 0, 0) == 0);
  assert(audio_bitrate_add(&meter, 48000, 48000, 48000, 1, 48000));
  assert(meter.bytes * 8 * meter.rate / meter.samples == 320000);
  /* The meter accumulates the payload byte counts supplied by its caller. */
  assert(meter.bytes == 80000);
  meter.bytes = UINT64_MAX - 10;
  assert(audio_bitrate_add(&meter, 1000, 1024, 48000, 2, 96000) == AUDIO_BITRATE_RESET);
  assert(meter.bytes == 1000 && meter.samples == 1024);

  /* ALAC UDP recovery is out of order. Neither recovery nor duplicate resends
   * should erase a window or inflate compressed bytes and input frames. */
  audio_bitrate_reset(&meter);
  assert(!audio_bitrate_add(&meter, 700, 352, 44100, 65534, 0xfffffd40));
  assert(!audio_bitrate_add(&meter, 900, 352, 44100, 0, 0));
  assert(!audio_bitrate_add(&meter, 800, 352, 44100, 65535, 0xfffffea0));
  assert(!audio_bitrate_add(&meter, 700, 352, 44100, 65534, 0xfffffd40));
  assert(meter.bytes == 2400 && meter.samples == 1056);
  assert(!audio_bitrate_add(&meter, 1000, 352, 44100, 1, 352));
  assert(!audio_bitrate_add(&meter, 800, 352, 44100, 65535, 0xfffffea0));
  assert(meter.bytes == 3400 && meter.samples == 1408);

  /* Every slot in the player window can be retransmitted. */
  audio_bitrate_reset(&meter);
  for (unsigned i = 0; i < AUDIO_BITRATE_HISTORY_SIZE; i++)
    audio_bitrate_add(&meter, 1000 + i, 352, 44100, i, i * 352);
  bytes = meter.bytes;
  samples = meter.samples;
  for (unsigned i = AUDIO_BITRATE_HISTORY_SIZE; i-- > 0;)
    assert(!audio_bitrate_add(&meter, 1000 + i, 352, 44100, i, i * 352));
  assert(meter.bytes == bytes && meter.samples == samples);
  /* Reusing a slot or wrapping a sequence with a new timestamp is new audio. */
  audio_bitrate_add(&meter, 2000, 352, 44100, AUDIO_BITRATE_HISTORY_SIZE, samples);
  audio_bitrate_add(&meter, 3000, 352, 44100, 0, samples + 352);
  assert(meter.bytes == bytes + 5000 && meter.samples == samples + 704);

  /* Duplicate retransmission must not trigger an overflow reset. */
  meter.bytes = UINT64_MAX - 1;
  assert(!audio_bitrate_add(&meter, 3000, 352, 44100, 0, samples + 352));
  assert(meter.bytes == UINT64_MAX - 1);
  assert(audio_bitrate_add(&meter, 4000, 352, 44100, 1, samples + 704) == AUDIO_BITRATE_RESET);
  meter.samples = UINT64_MAX - 100;
  assert(audio_bitrate_add(&meter, 4000, 352, 44100, 2, samples + 1056) == AUDIO_BITRATE_RESET);
  assert(meter.samples == 352);
  /* Reserve enough headroom for the next reporting threshold too. */
  meter.samples = UINT64_MAX - 44100;
  assert(audio_bitrate_add(&meter, 4000, 352, 44100, 3, samples + 1408) == AUDIO_BITRATE_RESET);
  assert(meter.samples == 352);

  audio_bitrate_reset(&meter);
  assert(audio_bitrate_add(&meter, 220500, 88200, 44100, 0, 0) == AUDIO_BITRATE_REPORT);
  assert(meter.bytes * 8 * meter.rate / meter.samples == 882000);
  assert(audio_bitrate_add(&meter, 360000, 96000, 48000, 0, 0) ==
         (AUDIO_BITRATE_RESET | AUDIO_BITRATE_REPORT));
  assert(meter.bytes * 8 * meter.rate / meter.samples == 1440000);

  /* Zero and negative intervals are clamped and keep a stable 5-second cadence. */
  config.bitrate_stats_interval = 0.0;
  audio_bitrate_reset(&meter);
  reports = 0;
  for (unsigned i = 0; i < 470; i++)
    reports += !!audio_bitrate_add(&meter, 680 + i % 3, 1024, 48000, i, i * 1024);
  assert(reports == 2);

  config.bitrate_stats_interval = -2.0;
  audio_bitrate_reset(&meter);
  reports = 0;
  for (unsigned i = 0; i < 470; i++)
    reports += !!audio_bitrate_add(&meter, 680 + i % 3, 1024, 48000, i, i * 1024);
  assert(reports == 2);

  /* Receiver counter reports obey a positive interval and include current totals. */
  config.bitrate_stats_interval = 1.0;
  rtsp_conn_info conn = {0};
  init_conn_mutexes(&conn);
  conn.missing_packets = 7;
  conn.too_late_packets = 5;
  conn.resend_requests = 11;
  test_audio_bitrate_stub_reset_metadata();
  test_audio_bitrate_stub_set_time_ns(1000000000ULL);
  report_player_counters(&conn);
  assert(test_audio_bitrate_stub_send_count() == 1);
  assert(test_audio_bitrate_stub_last_code() == METADATA_FOURCC('a', 'r', 's', 't'));
  assert(strcmp(test_audio_bitrate_stub_last_payload(), "7/5/11") == 0);

  test_audio_bitrate_stub_set_time_ns(1500000000ULL);
  report_player_counters(&conn);
  assert(test_audio_bitrate_stub_send_count() == 1);

  test_audio_bitrate_stub_set_time_ns(2100000000ULL);
  report_player_counters(&conn);
  assert(test_audio_bitrate_stub_send_count() == 2);

  config.bitrate_stats_interval = 0.0;
  test_audio_bitrate_stub_set_time_ns(3200000000ULL);
  report_player_counters(&conn);
  assert(test_audio_bitrate_stub_send_count() == 2);

  config.bitrate_stats_interval = -1.0;
  test_audio_bitrate_stub_set_time_ns(4300000000ULL);
  report_player_counters(&conn);
  assert(test_audio_bitrate_stub_send_count() == 2);
  destroy_conn_mutexes(&conn);

  /* Generation/accounting path: current generation only, with reset on timestamp discontinuity. */
  config.bitrate_stats_interval = 1.0;
  memset(&conn, 0, sizeof(conn));
  init_conn_mutexes(&conn);
  test_audio_bitrate_stub_reset_metadata();

  uint64_t generation = input_bitrate_packet_generation(&conn, ALAC_44100_S16_2, 10, 3520, 0);
  assert(generation == 1);
  assert(conn.input_bitrate_generation == 1);
  assert(conn.input_bitrate_ssrc == ALAC_44100_S16_2);
  account_input_bitrate(&conn, ALAC_44100_S16_2, 1000, 352, 10, 3520, generation);
  assert(conn.input_bitrate.bytes == 1000 && conn.input_bitrate.samples == 352);

  /* Stale generation token is ignored. */
  account_input_bitrate(&conn, ALAC_44100_S16_2, 500, 352, 11, 3872, generation - 1);
  assert(conn.input_bitrate.bytes == 1000 && conn.input_bitrate.samples == 352);

  /* Flush state pauses accounting and generation changes. */
  conn.flush_requested = 1;
  assert(input_bitrate_packet_generation(&conn, ALAC_44100_S16_2, 11, 3872, 0) == 1);
  account_input_bitrate(&conn, ALAC_44100_S16_2, 500, 352, 11, 3872, generation);
  assert(conn.input_bitrate.bytes == 1000 && conn.input_bitrate.samples == 352);
  conn.flush_requested = 0;

  /* Non-duplicate timestamp-gap discontinuity resets the meter and emits 0/0/0 once. */
  generation = input_bitrate_packet_generation(&conn, ALAC_44100_S16_2, 12, 4224, 128);
  assert(generation == 2);
  assert(conn.input_bitrate.bytes == 0 && conn.input_bitrate.samples == 0);
  assert(test_audio_bitrate_stub_send_count() == 1);
  assert(test_audio_bitrate_stub_last_code() == METADATA_FOURCC('a', 'b', 'r', 't'));
  assert(strcmp(test_audio_bitrate_stub_last_payload(), "0/0/0") == 0);

  account_input_bitrate(&conn, ALAC_44100_S16_2, 1200, 352, 12, 4224, generation);
  assert(conn.input_bitrate.bytes == 1200 && conn.input_bitrate.samples == 352);
  destroy_conn_mutexes(&conn);

  config.bitrate_stats_interval = 1.25;
  printf("Audio bitrate tests passed; persistent state=%zu bytes\n", sizeof(meter));
}
