#include <pthread.h>
#include <stdarg.h>
#include <unistd.h>

void _debug(__attribute__((unused)) const char *filename,
            __attribute__((unused)) const int linenumber,
            __attribute__((unused)) int level,
            __attribute__((unused)) const char *format, ...) {
}

void mutex_unlock(void *arg) { pthread_mutex_unlock((pthread_mutex_t *)arg); }

void socket_cleanup(void *arg) {
  int *fd = (int *)arg;
  if ((fd != NULL) && (*fd >= 0))
    close(*fd);
}
