#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "utilities/debug.h"

volatile int command_execute_call_count = 0;
volatile int command_execute_last_block = -1;
const char *command_execute_last_command = NULL;

int named_pthread_create_call_count = 0;
static int named_pthread_create_real_mode = 0;

shairport_cfg config;

void reset_command_execute_stub(void) {
  command_execute_call_count = 0;
  command_execute_last_block = -1;
  command_execute_last_command = NULL;
}

void reset_thread_lifecycle_stubs(void) {
  named_pthread_create_call_count = 0;
  named_pthread_create_real_mode = 0;
}

void set_named_pthread_create_real_mode(int enabled) {
  named_pthread_create_real_mode = enabled;
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
  named_pthread_create_call_count++;
  (void)format;
  if (named_pthread_create_real_mode)
    return pthread_create(thread, attr, start_routine, arg);

  (void)thread;
  (void)attr;
  (void)start_routine;
  (void)arg;
  return 0;
}

uint64_t get_realtime_in_ns(void) { return 0; }

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
