#include <stdarg.h>
#include <string.h>

#include "common.h"
#include "metadata/hub.h"
#include "rtsp.h"

struct metadata_bundle metadata_store;
rtsp_conn_info *principal_conn = NULL;
pthread_rwlock_t principal_conn_lock = PTHREAD_RWLOCK_INITIALIZER;

int remote_test_send_simple_dacp_command_call_count = 0;
char remote_test_last_simple_dacp_command[256];
int remote_test_dacp_set_integer_percent_volume_call_count = 0;
int remote_test_last_dacp_integer_percent_volume = -1;
int remote_test_stop_play_call_count = 0;

void remote_test_reset_stubs(void) {
  memset(&metadata_store, 0, sizeof(metadata_store));
  principal_conn = NULL;
  remote_test_send_simple_dacp_command_call_count = 0;
  remote_test_last_simple_dacp_command[0] = '\0';
  remote_test_dacp_set_integer_percent_volume_call_count = 0;
  remote_test_last_dacp_integer_percent_volume = -1;
  remote_test_stop_play_call_count = 0;
}

int send_simple_dacp_command(const char *command) {
  remote_test_send_simple_dacp_command_call_count++;
  if (command != NULL) {
    strncpy(remote_test_last_simple_dacp_command, command,
            sizeof(remote_test_last_simple_dacp_command) - 1);
    remote_test_last_simple_dacp_command[sizeof(remote_test_last_simple_dacp_command) - 1] = '\0';
  } else {
    remote_test_last_simple_dacp_command[0] = '\0';
  }
  return 204;
}

void dacp_set_integer_percent_volume(const int volume) {
  remote_test_dacp_set_integer_percent_volume_call_count++;
  remote_test_last_dacp_integer_percent_volume = volume;
}

void stop_play(void) { remote_test_stop_play_call_count++; }

void rwlock_unlock(void *arg) {
  pthread_rwlock_t *rwlock = (pthread_rwlock_t *)arg;
  pthread_rwlock_unlock(rwlock);
}

void _debug(__attribute__((unused)) const char *filename, __attribute__((unused)) const int linenumber,
            __attribute__((unused)) int level, __attribute__((unused)) const char *format, ...) {}

void _inform(__attribute__((unused)) const char *filename, __attribute__((unused)) const int linenumber,
             __attribute__((unused)) const char *format, ...) {}

void _warn(__attribute__((unused)) const char *filename, __attribute__((unused)) const int linenumber,
           __attribute__((unused)) const char *format, ...) {}

void _die(__attribute__((unused)) const char *filename, __attribute__((unused)) const int linenumber,
          __attribute__((unused)) const char *format, ...) {}
