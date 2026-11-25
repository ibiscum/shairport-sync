/* Focused regression tests for metadata/core.c branch behavior.
 * Uses lightweight stubs for outputs and rtsp/common dependencies. */
#define _POSIX_C_SOURCE 200809L
#undef NDEBUG
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define _COMMON_H
#define _RTSP_H
#define CONFIG_METADATA_PIPE 1
#define CONFIG_METADATA_MULTICAST 1
#define CONFIG_MQTT 1
#undef CONFIG_METADATA_HUB
#undef debug
#define debug(...) ((void)0)
#undef die
#define die(...) abort()

typedef struct {
  int metadata_enabled;
} shairport_cfg;
shairport_cfg config = {.metadata_enabled = 1};

typedef struct {
  int index_number;
  unsigned referenceCount;
} rtsp_message;

static unsigned retained_messages;
static unsigned released_messages;

static void *memdup(const void *data, size_t length) {
  void *copy = malloc(length);
  if (copy != NULL)
    memcpy(copy, data, length);
  return copy;
}

static void msg_retain(rtsp_message *message) {
  retained_messages++;
  message->referenceCount++;
}

static void msg_free(rtsp_message **message) {
  released_messages++;
  (*message)->referenceCount--;
}

static unsigned pipe_init_calls, pipe_stop_calls;
static unsigned multicast_init_calls, multicast_stop_calls;
static unsigned mqtt_init_calls, mqtt_stop_calls;
static unsigned pipe_send_calls, multicast_send_calls, mqtt_send_calls;
static int pipe_send_rc, multicast_send_rc, mqtt_send_rc;
static uint32_t pipe_last_type, pipe_last_code;

void metadata_pipe_queue_init(void) { pipe_init_calls++; }
void metadata_pipe_queue_stop(void) { pipe_stop_calls++; }
int send_metadata_to_pipe_queue(__attribute__((unused)) const uint32_t type,
                                __attribute__((unused)) const uint32_t code,
                                __attribute__((unused)) const char *data,
                                __attribute__((unused)) const uint32_t length,
                                __attribute__((unused)) rtsp_message *carrier,
                                __attribute__((unused)) int block) {
  pipe_send_calls++;
  pipe_last_type = type;
  pipe_last_code = code;
  return pipe_send_rc;
}

void metadata_multicast_queue_init(void) { multicast_init_calls++; }
void metadata_multicast_queue_stop(void) { multicast_stop_calls++; }
int send_metadata_to_multicast_queue(__attribute__((unused)) const uint32_t type,
                                     __attribute__((unused)) const uint32_t code,
                                     __attribute__((unused)) const char *data,
                                     __attribute__((unused)) const uint32_t length,
                                     __attribute__((unused)) rtsp_message *carrier,
                                     __attribute__((unused)) int block) {
  multicast_send_calls++;
  return multicast_send_rc;
}

void metadata_mqtt_queue_init(void) { mqtt_init_calls++; }
void metadata_mqtt_queue_stop(void) { mqtt_stop_calls++; }
int send_metadata_to_mqtt_queue(__attribute__((unused)) const uint32_t type,
                                __attribute__((unused)) const uint32_t code,
                                __attribute__((unused)) const char *data,
                                __attribute__((unused)) const uint32_t length,
                                __attribute__((unused)) rtsp_message *carrier,
                                __attribute__((unused)) int block) {
  mqtt_send_calls++;
  return mqtt_send_rc;
}

#include "../metadata/pc_queue.c"
#include "../metadata/core.c"

static void reset_counters(void) {
  retained_messages = 0;
  released_messages = 0;
  pipe_init_calls = pipe_stop_calls = 0;
  multicast_init_calls = multicast_stop_calls = 0;
  mqtt_init_calls = mqtt_stop_calls = 0;
  pipe_send_calls = multicast_send_calls = mqtt_send_calls = 0;
  pipe_send_rc = multicast_send_rc = mqtt_send_rc = 0;
  pipe_last_type = pipe_last_code = 0;
}

static void test_init_stop_idempotence(void) {
  reset_counters();
  metadata_running = 0;
  metadata_init();
  metadata_init();
  assert(metadata_running == 1);
  assert(pipe_init_calls == 1);
  assert(multicast_init_calls == 1);
  assert(mqtt_init_calls == 1);

  metadata_stop();
  metadata_stop();
  assert(metadata_running == 0);
  assert(pipe_stop_calls == 1);
  assert(multicast_stop_calls == 1);
  assert(mqtt_stop_calls == 1);
}

static void test_send_metadata_disabled_does_not_fanout(void) {
  reset_counters();
  config.metadata_enabled = 0;
  assert(send_metadata(METADATA_FOURCC('s', 's', 'n', 'c'),
                       METADATA_FOURCC('t', 'e', 's', 't'), "x", 1, NULL, 0) == 0);
  assert(pipe_send_calls == 0 && multicast_send_calls == 0 && mqtt_send_calls == 0);
  config.metadata_enabled = 1;
}

static void test_send_metadata_first_error_wins(void) {
  reset_counters();
  config.metadata_enabled = 1;
  pipe_send_rc = 0;
  multicast_send_rc = EWOULDBLOCK;
  mqtt_send_rc = EINVAL;
  assert(send_metadata(METADATA_FOURCC('s', 's', 'n', 'c'),
                       METADATA_FOURCC('t', 'e', 's', 't'), "x", 1, NULL, 0) == EWOULDBLOCK);
  assert(pipe_send_calls == 1 && multicast_send_calls == 1 && mqtt_send_calls == 1);
}

static void test_send_ssnc_metadata_sets_ssnc_type(void) {
  reset_counters();
  config.metadata_enabled = 1;
  assert(send_ssnc_metadata(METADATA_FOURCC('a', 'b', 'r', 't'), "x", 1, 0) == 0);
  assert(pipe_send_calls == 1);
  assert(pipe_last_type == METADATA_FOURCC('s', 's', 'n', 'c'));
  assert(pipe_last_code == METADATA_FOURCC('a', 'b', 'r', 't'));
}

static void test_send_metadata_to_queue_invalid_null_data(void) {
  pc_queue queue;
  metadata_package storage[1];
  pc_queue_init(&queue, (char *)storage, sizeof(metadata_package), 1, "core-regression");
  assert(send_metadata_to_queue(&queue, METADATA_FOURCC('s', 's', 'n', 'c'),
                                METADATA_FOURCC('t', 'e', 's', 't'), NULL, 3, NULL, 0) ==
         EINVAL);
  assert(queue.count == 0);
  pc_queue_delete(&queue);
}

static void test_carrier_retain_release_paths(void) {
  pc_queue queue;
  metadata_package storage[1];
  metadata_package item;
  rtsp_message msg = {.index_number = 9, .referenceCount = 1};
  reset_counters();
  pc_queue_init(&queue, (char *)storage, sizeof(metadata_package), 1, "core-regression");

  /* Success path retains, queue consumer releases. */
  assert(send_metadata_to_queue(&queue, METADATA_FOURCC('s', 's', 'n', 'c'),
                                METADATA_FOURCC('o', 'k', 'a', 'y'), "abc", 3, &msg, 0) == 0);
  assert(retained_messages == 1 && released_messages == 0);
  assert(msg.referenceCount == 2);
  assert(pc_queue_get_item(&queue, &item) == 0);
  metadata_pack_cleanup_function(&item);
  assert(retained_messages == 1 && released_messages == 1);
  assert(msg.referenceCount == 1);

  /* Full queue path releases immediately on enqueue failure. */
  assert(send_metadata_to_queue(&queue, METADATA_FOURCC('s', 's', 'n', 'c'),
                                METADATA_FOURCC('f', 'u', 'l', 'l'), "abc", 3, NULL, 0) == 0);
  assert(send_metadata_to_queue(&queue, METADATA_FOURCC('s', 's', 'n', 'c'),
                                METADATA_FOURCC('f', 'a', 'i', 'l'), "abc", 3, &msg, 0) ==
         EWOULDBLOCK);
  assert(retained_messages == 2 && released_messages == 2);
  assert(msg.referenceCount == 1);
  assert(pc_queue_get_item(&queue, &item) == 0);
  metadata_pack_cleanup_function(&item);
  pc_queue_delete(&queue);
}

int main(void) {
  test_init_stop_idempotence();
  test_send_metadata_disabled_does_not_fanout();
  test_send_metadata_first_error_wins();
  test_send_ssnc_metadata_sets_ssnc_type();
  test_send_metadata_to_queue_invalid_null_data();
  test_carrier_retain_release_paths();
  puts("Metadata core tests passed: lifecycle, fanout, validation and carrier retention paths.");
}
