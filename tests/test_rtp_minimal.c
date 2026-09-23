#include <arpa/inet.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>

#include <cmocka.h>

#include "common.h"
#include "player.h"
#include "rtp.h"
#include "test_rtp_minimal_stubs.h"

extern int32_t decipher_player_put_packet(uint8_t *ciphered_audio_alt,
                                          ssize_t nread, rtsp_conn_info *conn);

static void test_rtp_initialise_and_terminate(void **state) {
  (void)state;
  rtsp_conn_info conn;
  memset(&conn, 0, sizeof(conn));

  rtp_initialise(&conn);

  assert_int_equal(conn.rtp_running, 0);
  assert_int_equal(conn.rtp_time_of_last_resend_request_error_ns, 0);
  assert_int_equal(pthread_mutex_lock(&conn.reference_time_mutex), 0);
  assert_int_equal(pthread_mutex_unlock(&conn.reference_time_mutex), 0);

  rtp_terminate(&conn);
}

static void test_ntp_conversion_round_trip(void **state) {
  (void)state;
  rtsp_conn_info conn;
  memset(&conn, 0, sizeof(conn));

  rtp_initialise(&conn);

  conn.anchor_remote_info_is_valid = 1;
  conn.anchor_rtptime = 1000;
  conn.anchor_time = 2000000000ULL;
  conn.input_rate = 44100;
  conn.local_to_remote_time_difference = 1000000ULL;
  conn.local_to_remote_time_gradient = 1.0;
  set_mock_absolute_time_ns(123456789ULL);
  conn.local_to_remote_time_difference_measurement_time = 123456789ULL;

#ifdef CONFIG_AIRPLAY_2
  conn.timing_type = ts_ntp;
#endif

  uint32_t frame = conn.anchor_rtptime + conn.input_rate;
  uint64_t local_time = 0;
  assert_int_equal(frame_to_local_time(frame, &local_time, &conn), 0);
  assert_int_equal(local_time, 2999000000ULL);

  uint32_t frame_back = 0;
  assert_int_equal(local_time_to_frame(local_time, &frame_back, &conn), 0);
  assert_int_equal(frame_back, frame);

  assert_int_equal(have_timestamp_timing_information(&conn), 1);

  rtp_terminate(&conn);
}

static void test_reset_anchor_info_clears_ntp_validity(void **state) {
  (void)state;
  rtsp_conn_info conn;
  memset(&conn, 0, sizeof(conn));

  rtp_initialise(&conn);

  conn.anchor_remote_info_is_valid = 1;
  conn.anchor_rtptime = 42;
  conn.anchor_time = 77;
#ifdef CONFIG_AIRPLAY_2
  conn.timing_type = ts_ntp;
#endif

  reset_anchor_info(&conn);
  assert_int_equal(have_timestamp_timing_information(&conn), 0);
  assert_int_equal(conn.anchor_remote_info_is_valid, 0);
  assert_int_equal(conn.anchor_rtptime, 0);
  assert_int_equal(conn.anchor_time, 0);

  rtp_terminate(&conn);
}

static void test_rtp_setup_marks_running_and_binds_ports(void **state) {
  (void)state;
  rtsp_conn_info conn;
  memset(&conn, 0, sizeof(conn));

  rtp_initialise(&conn);
  set_mock_bind_port_base(7000);

  SOCKADDR local;
  SOCKADDR remote;
  memset(&local, 0, sizeof(local));
  memset(&remote, 0, sizeof(remote));

  struct sockaddr_in *local4 = (struct sockaddr_in *)&local;
  struct sockaddr_in *remote4 = (struct sockaddr_in *)&remote;
  local4->sin_family = AF_INET;
  remote4->sin_family = AF_INET;
  assert_int_equal(inet_pton(AF_INET, "127.0.0.1", &local4->sin_addr), 1);
  assert_int_equal(inet_pton(AF_INET, "127.0.0.1", &remote4->sin_addr), 1);

  rtp_setup(&local, &remote, 6001, 6002, &conn);

  assert_int_equal(conn.rtp_running, 1);
  assert_int_equal(conn.local_control_port, 7000);
  assert_int_equal(conn.local_timing_port, 7001);
  assert_int_equal(conn.local_audio_port, 7002);
  assert_string_equal(conn.client_ip_string, "127.0.0.1");
  assert_string_equal(conn.self_ip_string, "127.0.0.1");

  rtp_terminate(&conn);
}

static void test_resend_when_not_running_sends_nothing(void **state) {
  (void)state;
  rtsp_conn_info conn;
  memset(&conn, 0, sizeof(conn));

  rtp_initialise(&conn);
  conn.rtp_running = 0;

  rtp_request_resend(100, 2, &conn);

  assert_int_equal(sendto_call_count, 0);

  rtp_terminate(&conn);
}

static void test_resend_running_sends_request(void **state) {
  (void)state;
  rtsp_conn_info conn;
  memset(&conn, 0, sizeof(conn));

  rtp_initialise(&conn);
  conn.rtp_running = 1;
  conn.control_socket = 10;
  conn.rtp_client_control_socket.SAFAMILY = AF_INET;
  config.diagnostic_drop_packet_fraction = 0.0;

  set_mock_absolute_time_ns(1000000000ULL);
  rtp_request_resend(200, 4, &conn);

  assert_int_equal(sendto_call_count, 1);
  assert_int_equal(conn.rtp_time_of_last_resend_request_error_ns, 0);

  rtp_terminate(&conn);
}

static void test_resend_backoff_after_send_failure(void **state) {
  (void)state;
  rtsp_conn_info conn;
  memset(&conn, 0, sizeof(conn));

  rtp_initialise(&conn);
  conn.rtp_running = 1;
  conn.control_socket = 10;
  conn.rtp_client_control_socket.SAFAMILY = AF_INET;
  config.diagnostic_drop_packet_fraction = 0.0;

  set_mock_sendto_response(1);
  set_mock_absolute_time_ns(1000ULL);
  rtp_request_resend(10, 1, &conn);
  assert_int_equal(sendto_call_count, 1);
  assert_int_equal(conn.rtp_time_of_last_resend_request_error_ns, 1000ULL);

  set_mock_absolute_time_ns(1000ULL + 100000000ULL);
  rtp_request_resend(11, 1, &conn);
  assert_int_equal(sendto_call_count, 1);

  set_mock_absolute_time_ns(1000ULL + 400000000ULL);
  rtp_request_resend(12, 1, &conn);
  assert_int_equal(sendto_call_count, 2);

  rtp_terminate(&conn);
}

static void test_decipher_packet_too_short_is_rejected(void **state) {
  (void)state;
  rtsp_conn_info conn;
  memset(&conn, 0, sizeof(conn));
  uint8_t packet[18] = {0};

  int32_t result = decipher_player_put_packet(packet, sizeof(packet), &conn);
  assert_int_equal(result, -1);
  assert_int_equal(player_put_packet_call_count, 0);
}

static void test_decipher_without_session_key_skips_ingest(void **state) {
  (void)state;
  rtsp_conn_info conn;
  memset(&conn, 0, sizeof(conn));
  uint8_t packet[24] = {0};
  uint16_t seq = htons(321);
  uint32_t ts = htonl(654321);
  memcpy(packet, &seq, sizeof(seq));
  memcpy(packet + 2, &ts, sizeof(ts));

  int32_t result = decipher_player_put_packet(packet, sizeof(packet), &conn);
  assert_int_equal(result, 321);
  assert_int_equal(player_put_packet_call_count, 0);
}

static void test_decipher_with_session_key_ingests_payload(void **state) {
  (void)state;
  rtsp_conn_info conn;
  memset(&conn, 0, sizeof(conn));
  uint8_t packet[40] = {0};
  uint16_t seq = htons(2222);
  uint32_t ts = htonl(123456);
  unsigned char fake_key[32] = {0};

  memcpy(packet, &seq, sizeof(seq));
  memcpy(packet + 2, &ts, sizeof(ts));
  conn.session_key = fake_key;

  int32_t result = decipher_player_put_packet(packet, sizeof(packet), &conn);
  assert_int_equal(result, 2222);
  assert_int_equal(player_put_packet_call_count, 1);
  assert_int_equal(player_put_packet_last_seq, 2222);
  assert_int_equal(player_put_packet_last_timestamp, 123456);
  assert_int_equal(player_put_packet_last_len, 6);
}

#ifdef CONFIG_AIRPLAY_2
static void test_resend_ap2_without_remote_socket_skips_send(void **state) {
  (void)state;
  rtsp_conn_info conn;
  memset(&conn, 0, sizeof(conn));

  rtp_initialise(&conn);
  conn.rtp_running = 1;
  conn.airplay_type = ap_2;
  conn.ap2_remote_control_socket_addr_length = 0;

  rtp_request_resend(5, 2, &conn);
  assert_int_equal(sendto_call_count, 0);

  rtp_terminate(&conn);
}

static void test_set_and_reset_ptp_anchor_info(void **state) {
  (void)state;
  rtsp_conn_info conn;
  memset(&conn, 0, sizeof(conn));

  conn.connection_number = 99;
  set_ptp_anchor_info(&conn, 0x1234ULL, 1000U, 2000ULL);
  assert_int_equal(conn.anchor_remote_info_is_valid, 1);
  assert_int_equal(conn.anchor_clock, 0x1234ULL);
  assert_int_equal(conn.anchor_rtptime, 1000U);
  assert_int_equal(conn.anchor_time, 2000ULL);

  conn.last_anchor_info_is_valid = 1;
  reset_ptp_anchor_info(&conn);
  assert_int_equal(conn.anchor_remote_info_is_valid, 0);
  assert_int_equal(conn.last_anchor_info_is_valid, 0);
}

static void test_get_ptp_anchor_local_time_info_clock_ok(void **state) {
  (void)state;
  rtsp_conn_info conn;
  memset(&conn, 0, sizeof(conn));

  const uint64_t now = 10000000000ULL;
  conn.anchor_remote_info_is_valid = 1;
  conn.anchor_clock = 0xABCDULL;
  conn.anchor_rtptime = 44100U;
  conn.anchor_time = 5000000000ULL;
  conn.input_rate = 44100;

  set_mock_absolute_time_ns(now);
  set_mock_ptp_response(clock_ok, conn.anchor_clock, now - 100000000ULL, 2000ULL,
                        now - 1000000000ULL);

  uint32_t anchor_rtp = 0;
  uint64_t anchor_local = 0;
  int response = get_ptp_anchor_local_time_info(&conn, &anchor_rtp, &anchor_local);

  assert_int_equal(response, clock_ok);
  assert_int_equal(conn.last_anchor_info_is_valid, 1);
  assert_int_equal(anchor_rtp, conn.anchor_rtptime);
  assert_int_equal(anchor_local, conn.anchor_time - 2000ULL);
}
#endif

static int setup(void **state) {
  (void)state;
  reset_rtp_test_stubs();
  return 0;
}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test_setup(test_rtp_initialise_and_terminate, setup),
      cmocka_unit_test_setup(test_ntp_conversion_round_trip, setup),
      cmocka_unit_test_setup(test_reset_anchor_info_clears_ntp_validity, setup),
      cmocka_unit_test_setup(test_rtp_setup_marks_running_and_binds_ports, setup),
      cmocka_unit_test_setup(test_resend_when_not_running_sends_nothing, setup),
    cmocka_unit_test_setup(test_resend_running_sends_request, setup),
    cmocka_unit_test_setup(test_resend_backoff_after_send_failure, setup),
    cmocka_unit_test_setup(test_decipher_packet_too_short_is_rejected, setup),
    cmocka_unit_test_setup(test_decipher_without_session_key_skips_ingest, setup),
    cmocka_unit_test_setup(test_decipher_with_session_key_ingests_payload, setup),
  #ifdef CONFIG_AIRPLAY_2
    cmocka_unit_test_setup(test_resend_ap2_without_remote_socket_skips_send, setup),
    cmocka_unit_test_setup(test_set_and_reset_ptp_anchor_info, setup),
    cmocka_unit_test_setup(test_get_ptp_anchor_local_time_info_clock_ok, setup),
  #endif
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
