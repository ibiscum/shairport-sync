#pragma once

// Keep tinysvcmdns.c buildable for focused unit tests without pulling full runtime deps.
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#ifndef _COMMON_H
#define _COMMON_H
#endif

#include <pthread.h>
#include <syslog.h>

#ifndef SOCKET_ERROR
#define SOCKET_ERROR -1
#endif

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

typedef struct {
  char *name;
  int (*mdns_register)(char *ap1name, char *ap2name, int port, char **txt_records,
                       char **secondary_txt_records);
  int (*mdns_update)(char **txt_records, char **secondary_txt_records);
  void (*mdns_unregister)(void);
  void (*mdns_dacp_monitor_start)(void);
  void (*mdns_dacp_monitor_set_id)(const char *);
  void (*mdns_dacp_monitor_stop)(void);
} mdns_backend;

void _debug(const char *filename, const int linenumber, int level, const char *format, ...);
void _inform(const char *filename, const int linenumber, const char *format, ...);
void _warn(const char *filename, const int linenumber, const char *format, ...);
void _die(const char *filename, const int linenumber, const char *format, ...);

#define debug(level, ...) _debug(__FILE__, __LINE__, level, __VA_ARGS__)
#define inform(...) _inform(__FILE__, __LINE__, __VA_ARGS__)
#define warn(...) _warn(__FILE__, __LINE__, __VA_ARGS__)
#define die(...) _die(__FILE__, __LINE__, __VA_ARGS__)

int named_pthread_create(pthread_t *restrict thread, const pthread_attr_t *restrict attr,
                         void *(*start_routine)(void *), void *restrict arg,
                         const char *format, ...);
void mutex_unlock(void *arg);
