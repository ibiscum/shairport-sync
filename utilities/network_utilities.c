/*
 * Network Utilities. This file is part of Shairport Sync.
 * Copyright (c) Mike Brady 2026
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "network_utilities.h"
#include "common.h"
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int eintr_checked_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
  int response;
  do {
    response = accept(sockfd, addr, addrlen);

    if (response == -1) {
  int err = errno;
      char errorstring[1024];
#if defined(__GLIBC__) && defined(__USE_GNU)
  char *errp = strerror_r(err, (char *)errorstring, sizeof(errorstring));
  const char *errmsg = (errp != NULL) ? errp : "unknown error";
#else
  const char *errmsg = errorstring;
  if (strerror_r(err, (char *)errorstring, sizeof(errorstring)) != 0)
    snprintf(errorstring, sizeof(errorstring), "error %d", err);
#endif
  debug(1, "error %d accept()ing a socket %d: \"%s\". (Note: error %d will be ignored.)", err,
    sockfd, errmsg, EINTR);
  errno = err;
    }

  } while ((response == -1) && (errno == EINTR));
  return response;
}

pthread_mutex_t safe_socket_lock = PTHREAD_MUTEX_INITIALIZER;

int _safe_socket_close(const char *filename, const int linenumber, int *sockfd) {
  int result = 0;

  if (sockfd == NULL) {
    errno = EINVAL;
    _debug(filename, linenumber, 1, "_safe_socket_close: sockfd pointer is NULL!");
    return -1;
  }

  int oldstate;
  pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldstate);
  pthread_mutex_lock(&safe_socket_lock);
  if (*sockfd >= 0) {
    int fd_to_close = *sockfd;
    _debug(filename, linenumber, 4, "_safe_socket_close: closing socket %d.", fd_to_close);
    *sockfd = -1;
    result = close(fd_to_close);
  } else {
    _debug(filename, linenumber, 1, "_safe_socket_close: socket already closed!");
  }
  pthread_mutex_unlock(&safe_socket_lock);
  pthread_setcancelstate(oldstate, NULL);
  return result;
}