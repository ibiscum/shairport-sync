#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <mosquitto.h>

#include "common.h"
#include "metadata/core.h"
#include "metadata/pc_queue.h"
#include "remote/remote.h"

shairport_cfg config;

typedef struct {
  char topic[256];
  char payload[256];
  int payload_len;
} mqtt_test_publish_event;

static struct mosquitto *test_mosq = NULL;
static void (*test_message_callback)(struct mosquitto *, void *, const struct mosquitto_message *) = NULL;
static void (*test_connect_callback)(struct mosquitto *, void *, int) = NULL;
static int test_connect_return_value = 0;

static mqtt_test_publish_event test_publish_events[32];
static int test_publish_event_count = 0;

int mqtt_test_remote_command_call_count = 0;
simple_command_t mqtt_test_last_remote_command = rcsc_not_a_command;

void mqtt_test_reset_stubs(void) {
  test_message_callback = NULL;
  test_connect_callback = NULL;
  test_connect_return_value = 0;
  test_publish_event_count = 0;
  mqtt_test_remote_command_call_count = 0;
  mqtt_test_last_remote_command = rcsc_not_a_command;
}

void mqtt_test_set_connect_return_value(int value) { test_connect_return_value = value; }

int mqtt_test_deliver_message(const char *topic, const char *payload) {
  if (test_message_callback == NULL)
    return -1;
  struct mosquitto_message msg = {0};
  msg.topic = (char *)topic;
  msg.payload = (void *)payload;
  msg.payloadlen = (int)strlen(payload);
  test_message_callback(test_mosq, NULL, &msg);
  return 0;
}

int mqtt_test_get_publish_count(void) { return test_publish_event_count; }

const mqtt_test_publish_event *mqtt_test_get_publish_event(int index) {
  if ((index < 0) || (index >= test_publish_event_count))
    return NULL;
  return &test_publish_events[index];
}

void remote_simple_command(simple_command_t command) {
  mqtt_test_remote_command_call_count++;
  mqtt_test_last_remote_command = command;
}

int send_simple_dacp_command(__attribute__((unused)) const char *command) { return 204; }

char *str_replace(const char *str, const char *old, const char *new) {
  if ((str == NULL) || (old == NULL) || (new == NULL))
    return NULL;
  if (strcmp(old, ":") == 0 && strcmp(new, "") == 0) {
    size_t n = strlen(str);
    char *out = calloc(n + 1, 1);
    if (out == NULL)
      return NULL;
    size_t j = 0;
    for (size_t i = 0; i < n; i++) {
      if (str[i] != ':')
        out[j++] = str[i];
    }
    out[j] = '\0';
    return out;
  }
  return strdup(str);
}

char *get_version_string(void) { return "test-version"; }

void metadata_pack_cleanup_function(__attribute__((unused)) void *arg) {}

void metadata_queue_delete(__attribute__((unused)) pc_queue *queue) {}

void pc_queue_init(__attribute__((unused)) pc_queue *the_queue,
                   __attribute__((unused)) char *the_store,
                   __attribute__((unused)) size_t item_size,
                   __attribute__((unused)) uint32_t count_of_items,
                   __attribute__((unused)) const char *debug_name) {}

int pc_queue_get_item(__attribute__((unused)) pc_queue *the_queue,
                      __attribute__((unused)) void *the_item) {
  return -1;
}

int send_metadata_to_queue(__attribute__((unused)) pc_queue *the_queue,
                           __attribute__((unused)) const uint32_t type,
                           __attribute__((unused)) const uint32_t code,
                           __attribute__((unused)) const char *data,
                           __attribute__((unused)) const uint32_t length,
                           __attribute__((unused)) rtsp_message *carrier,
                           __attribute__((unused)) int block) {
  return 0;
}

int named_pthread_create(__attribute__((unused)) pthread_t *restrict thread,
                         __attribute__((unused)) const pthread_attr_t *restrict attr,
                         __attribute__((unused)) void *(*start_routine)(void *),
                         __attribute__((unused)) void *restrict arg,
                         __attribute__((unused)) const char *format, ...) {
  return 0;
}

int mosquitto_lib_init(void) { return MOSQ_ERR_SUCCESS; }

struct mosquitto *mosquitto_new(__attribute__((unused)) const char *id,
                                __attribute__((unused)) bool clean_session,
                                __attribute__((unused)) void *obj) {
  test_mosq = malloc(1);
  return test_mosq;
}

int mosquitto_tls_set(__attribute__((unused)) struct mosquitto *mosq,
                      __attribute__((unused)) const char *cafile,
                      __attribute__((unused)) const char *capath,
                      __attribute__((unused)) const char *certfile,
                      __attribute__((unused)) const char *keyfile,
                      __attribute__((unused)) int (*pw_callback)(char *buf, int size, int rwflag,
                                                                void *userdata)) {
  return MOSQ_ERR_SUCCESS;
}

int mosquitto_username_pw_set(__attribute__((unused)) struct mosquitto *mosq,
                              __attribute__((unused)) const char *username,
                              __attribute__((unused)) const char *password) {
  return MOSQ_ERR_SUCCESS;
}

void mosquitto_log_callback_set(__attribute__((unused)) struct mosquitto *mosq,
                                __attribute__((unused)) void (*on_log)(struct mosquitto *, void *,
                                                                        int, const char *)) {}

void mosquitto_message_callback_set(
    __attribute__((unused)) struct mosquitto *mosq,
    void (*on_message)(struct mosquitto *, void *, const struct mosquitto_message *)) {
  test_message_callback = on_message;
}

void mosquitto_disconnect_callback_set(__attribute__((unused)) struct mosquitto *mosq,
                                       __attribute__((unused)) void (*on_disconnect)(struct mosquitto *, void *, int)) {}

void mosquitto_connect_callback_set(__attribute__((unused)) struct mosquitto *mosq,
                                    void (*on_connect)(struct mosquitto *, void *, int)) {
  test_connect_callback = on_connect;
}

int mosquitto_connect(__attribute__((unused)) struct mosquitto *mosq,
                      __attribute__((unused)) const char *host,
                      __attribute__((unused)) int port,
                      __attribute__((unused)) int keepalive) {
  if (test_connect_callback != NULL)
    test_connect_callback(test_mosq, NULL, test_connect_return_value);
  return MOSQ_ERR_SUCCESS;
}

int mosquitto_loop_start(__attribute__((unused)) struct mosquitto *mosq) { return MOSQ_ERR_SUCCESS; }

int mosquitto_subscribe(__attribute__((unused)) struct mosquitto *mosq,
                        __attribute__((unused)) int *mid,
                        __attribute__((unused)) const char *sub,
                        __attribute__((unused)) int qos) {
  return MOSQ_ERR_SUCCESS;
}

int mosquitto_publish(__attribute__((unused)) struct mosquitto *mosq,
                      __attribute__((unused)) int *mid, const char *topic, int payloadlen,
                      const void *payload, __attribute__((unused)) int qos,
                      __attribute__((unused)) bool retain) {
  if (test_publish_event_count < (int)(sizeof(test_publish_events) / sizeof(test_publish_events[0]))) {
    mqtt_test_publish_event *event = &test_publish_events[test_publish_event_count++];
    strncpy(event->topic, topic, sizeof(event->topic) - 1);
    event->topic[sizeof(event->topic) - 1] = '\0';
    if ((payload != NULL) && (payloadlen > 0)) {
      size_t copy_len = (size_t)payloadlen;
      if (copy_len > sizeof(event->payload) - 1)
        copy_len = sizeof(event->payload) - 1;
      memcpy(event->payload, payload, copy_len);
      event->payload[copy_len] = '\0';
      event->payload_len = payloadlen;
    } else {
      event->payload[0] = '\0';
      event->payload_len = 0;
    }
  }
  return MOSQ_ERR_SUCCESS;
}

int mosquitto_disconnect(__attribute__((unused)) struct mosquitto *mosq) { return MOSQ_ERR_SUCCESS; }

int mosquitto_loop_stop(__attribute__((unused)) struct mosquitto *mosq,
                        __attribute__((unused)) bool force) {
  return MOSQ_ERR_SUCCESS;
}

void mosquitto_destroy(struct mosquitto *mosq) {
  free(mosq);
  test_mosq = NULL;
}

void _debug(const char *filename, const int linenumber, int level, const char *format, ...) {
  (void)filename;
  (void)linenumber;
  (void)level;
  (void)format;
}

void _inform(const char *filename, const int linenumber, const char *format, ...) {
  (void)filename;
  (void)linenumber;
  (void)format;
}

void _warn(const char *filename, const int linenumber, const char *format, ...) {
  (void)filename;
  (void)linenumber;
  (void)format;
}

void _die(const char *filename, const int linenumber, const char *format, ...) {
  (void)filename;
  (void)linenumber;
  (void)format;
  abort();
}
