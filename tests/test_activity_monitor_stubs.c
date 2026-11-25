#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "utilities/debug.h"

int command_execute_call_count = 0;
int command_execute_last_block = -1;
const char *command_execute_last_command = NULL;
int named_pthread_create_call_count = 0;
int named_pthread_create_return_value = 0;

shairport_cfg config;

void reset_command_execute_stub(void) {
  command_execute_call_count = 0;
  command_execute_last_block = -1;
  command_execute_last_command = NULL;
  named_pthread_create_call_count = 0;
  named_pthread_create_return_value = 0;
}

void command_execute(const char *command, const char *extra_argument,
                     const int block) {
  (void)extra_argument;
  command_execute_call_count++;
  command_execute_last_block = block;
  command_execute_last_command = command;
}

void mutex_unlock(void *arg) { pthread_mutex_unlock((pthread_mutex_t *)arg); }

int named_pthread_create(pthread_t *restrict thread,
                         const pthread_attr_t *restrict attr,
                         void *(*start_routine)(void *), void *restrict arg,
                         const char *format, ...) {
  (void)format;
  named_pthread_create_call_count++;
  if (named_pthread_create_return_value != 0)
    return named_pthread_create_return_value;
  return pthread_create(thread, attr, start_routine, arg);
}

uint64_t get_realtime_in_ns(void) { return 0; }

int send_ssnc_metadata(__attribute__((unused)) const uint32_t code,
                       __attribute__((unused)) const char *data,
                       __attribute__((unused)) const uint32_t length,
                       __attribute__((unused)) const int block) {
  return 0;
}

void _debug(const char *filename, const int linenumber, int level,
            const char *format, ...) {
  (void)filename;
  (void)linenumber;
  (void)level;
  (void)format;
}

void _inform(const char *filename, const int linenumber, const char *format,
             ...) {
  (void)filename;
  (void)linenumber;
  (void)format;
}

void _warn(const char *filename, const int linenumber, const char *format,
           ...) {
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
