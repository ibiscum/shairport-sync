#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "metadata/hub.h"
#include "tinyhttp/http.h"

shairport_cfg config;
struct metadata_bundle metadata_store;
metadata_watcher metadata_watchers[number_of_watchers];

int dacp_named_pthread_create_calls = 0;
int dacp_named_pthread_create_return_value = 0;

void dacp_test_reset_stubs(void) {
  dacp_named_pthread_create_calls = 0;
  dacp_named_pthread_create_return_value = 0;
}

void dacp_test_set_named_pthread_create_return_value(int value) {
  dacp_named_pthread_create_return_value = value;
}

static void *dummy_dacp_monitor_thread(void *arg) {
  (void)arg;
  while (1) {
    usleep(10000);
    pthread_testcancel();
  }
}

int named_pthread_create(pthread_t *restrict thread, const pthread_attr_t *restrict attr,
                         void *(*start_routine)(void *), void *restrict arg, const char *format,
                         ...) {
  (void)start_routine;
  (void)arg;
  (void)format;
  dacp_named_pthread_create_calls++;
  if (dacp_named_pthread_create_return_value != 0)
    return dacp_named_pthread_create_return_value;
  return pthread_create(thread, attr, dummy_dacp_monitor_thread, NULL);
}

void mdns_dacp_monitor_set_id(const char *dacp_id) { (void)dacp_id; }

int update_string_record_with_data(char **str, const char *data, size_t length) {
  char *replacement = NULL;
  if (data) {
    replacement = calloc(length + 1, 1);
    if (replacement == NULL)
      return 0;
    memcpy(replacement, data, length);
  }
  if (((*str == NULL) && (replacement == NULL)) ||
      ((*str != NULL) && (replacement != NULL) && (strcmp(*str, replacement) == 0))) {
    free(replacement);
    return 0;
  }
  free(*str);
  *str = replacement;
  return 1;
}

int update_uint64_record(uint64_record_t *record, const uint64_t value) {
  if ((record->valid != 0) && (record->value == value))
    return 0;
  record->value = value;
  record->valid = 1;
  return 1;
}

void _metadata_hub_modify_prolog(const char *filename, const int linenumber) {
  (void)filename;
  (void)linenumber;
}

void _metadata_hub_modify_epilog(int modified, const char *filename, const int linenumber) {
  (void)modified;
  (void)filename;
  (void)linenumber;
}

uint64_t get_realtime_in_ns(void) { return 0; }

uint64_t get_absolute_time_in_ns(void) { return 0; }

void getErrorText(char *destinationString, size_t destinationStringLength) {
  if ((destinationString == NULL) || (destinationStringLength == 0))
    return;
  strncpy(destinationString, "stub", destinationStringLength - 1);
  destinationString[destinationStringLength - 1] = '\0';
}

void socket_cleanup(void *arg) {
  int *fd = (int *)arg;
  if ((fd != NULL) && (*fd >= 0)) {
    close(*fd);
    *fd = -1;
  }
}

void malloc_cleanup(void *arg) {
  void **ptr = (void **)arg;
  if ((ptr != NULL) && (*ptr != NULL)) {
    free(*ptr);
    *ptr = NULL;
  }
}

void mutex_unlock(void *arg) {
  if (arg != NULL)
    pthread_mutex_unlock((pthread_mutex_t *)arg);
}

void http_init(struct http_roundtripper *rt, struct http_funcs funcs, void *opaque) {
  if (rt == NULL)
    return;
  memset(rt, 0, sizeof(*rt));
  rt->funcs = funcs;
  rt->opaque = opaque;
}

void http_free(__attribute__((unused)) struct http_roundtripper *rt) {}

int http_data(__attribute__((unused)) struct http_roundtripper *rt,
              __attribute__((unused)) const char *data, int size, int *read) {
  if (read)
    *read = size;
  return 0;
}

int http_iserror(__attribute__((unused)) struct http_roundtripper *rt) { return 0; }

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
