#include <stdarg.h>
#include <stdlib.h>

#include "test_tinysvcmdns_harness.h"

void mutex_unlock(void *arg) { pthread_mutex_unlock((pthread_mutex_t *)arg); }

int named_pthread_create(pthread_t *restrict thread,
                         const pthread_attr_t *restrict attr,
                         void *(*start_routine)(void *), void *restrict arg,
                         const char *format, ...) {
  (void)thread;
  (void)attr;
  (void)start_routine;
  (void)arg;
  (void)format;
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
