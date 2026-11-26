#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <cmocka.h>

#include "common.h"
#include "mqtt.h"
#include "remote/remote.h"

typedef struct {
  char topic[256];
  char payload[256];
  int payload_len;
} mqtt_test_publish_event;

extern void mqtt_test_reset_stubs(void);
extern void mqtt_test_set_connect_return_value(int value);
extern int mqtt_test_deliver_message(const char *topic, const char *payload);
extern int mqtt_test_get_publish_count(void);
extern const mqtt_test_publish_event *mqtt_test_get_publish_event(int index);
extern int mqtt_test_remote_command_call_count;
extern simple_command_t mqtt_test_last_remote_command;

static uint32_t make_fourcc(char a, char b, char c, char d) {
  return ((uint32_t)(uint8_t)a << 24) | ((uint32_t)(uint8_t)b << 16) |
         ((uint32_t)(uint8_t)c << 8) | (uint32_t)(uint8_t)d;
}

static int setup(void **state) {
  (void)state;
  memset(&config, 0, sizeof(config));
  mqtt_test_reset_stubs();
  config.service_name = "MQTT Regression";
  config.mqtt_hostname = "localhost";
  config.mqtt_port = 1883;
  config.mqtt_topic = "shairport/test";
  config.mqtt_enable_remote = 1;
  config.mqtt_publish_parsed = 1;
  config.mqtt_publish_raw = 0;
  config.mqtt_publish_cover = 0;
  config.mqtt_publish_retain = 0;
  config.mqtt_enable_autodiscovery = 0;
  mqtt_test_set_connect_return_value(0);
  assert_int_equal(initialise_mqtt(), 0);
  return 0;
}

static int teardown(void **state) {
  (void)state;
  metadata_mqtt_queue_stop();
  return 0;
}

static void test_remote_command_requires_strict_match(void **state) {
  (void)state;
  assert_int_equal(mqtt_test_deliver_message("shairport/test/remote", " playpause "), 0);
  assert_int_equal(mqtt_test_remote_command_call_count, 1);
  assert_int_equal(mqtt_test_last_remote_command, rcsc_play_pause);

  assert_int_equal(mqtt_test_deliver_message("shairport/test/remote", "playpauseX"), 0);
  assert_int_equal(mqtt_test_remote_command_call_count, 1);
}

static void test_core_metadata_maps_to_expected_topic(void **state) {
  (void)state;
  mqtt_process_metadata(make_fourcc('c', 'o', 'r', 'e'), make_fourcc('a', 's', 'a', 'r'), "ABBA",
                        4);

  assert_int_equal(mqtt_test_get_publish_count(), 1);
  const mqtt_test_publish_event *event = mqtt_test_get_publish_event(0);
  assert_non_null(event);
  assert_string_equal(event->topic, "shairport/test/artist");
  assert_string_equal(event->payload, "ABBA");
  assert_int_equal(event->payload_len, 4);
}

static void test_track_id_metadata_hex_encoding(void **state) {
  (void)state;
  const char track_id[] = {(char)0x01, (char)0xA2, (char)0x7F};
  mqtt_process_metadata(make_fourcc('c', 'o', 'r', 'e'), make_fourcc('m', 'p', 'e', 'r'), track_id,
                        sizeof(track_id));

  assert_int_equal(mqtt_test_get_publish_count(), 1);
  const mqtt_test_publish_event *event = mqtt_test_get_publish_event(0);
  assert_non_null(event);
  assert_string_equal(event->topic, "shairport/test/track_id");
  assert_string_equal(event->payload, "01A27F");
}

static void test_ssnc_play_begin_maps_playing_and_start(void **state) {
  (void)state;
  mqtt_process_metadata(make_fourcc('s', 's', 'n', 'c'), make_fourcc('p', 'b', 'e', 'g'), "start",
                        5);

  assert_int_equal(mqtt_test_get_publish_count(), 2);

  const mqtt_test_publish_event *first = mqtt_test_get_publish_event(0);
  const mqtt_test_publish_event *second = mqtt_test_get_publish_event(1);
  assert_non_null(first);
  assert_non_null(second);

  assert_string_equal(first->topic, "shairport/test/playing");
  assert_string_equal(first->payload, "1");
  assert_int_equal(first->payload_len, 1);

  assert_string_equal(second->topic, "shairport/test/play_start");
  assert_string_equal(second->payload, "start");
  assert_int_equal(second->payload_len, 5);
}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test_setup_teardown(test_remote_command_requires_strict_match, setup, teardown),
      cmocka_unit_test_setup_teardown(test_core_metadata_maps_to_expected_topic, setup, teardown),
      cmocka_unit_test_setup_teardown(test_track_id_metadata_hex_encoding, setup, teardown),
      cmocka_unit_test_setup_teardown(test_ssnc_play_begin_maps_playing_and_start, setup, teardown),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
