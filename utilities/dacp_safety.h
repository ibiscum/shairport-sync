#pragma once

#include <arpa/inet.h>
#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>
#include <string.h>

static inline void dacp_safe_copy_string(char *dst, size_t dst_size, const char *src) {
  if ((dst == NULL) || (dst_size == 0))
    return;

  if (src == NULL) {
    dst[0] = '\0';
    return;
  }

  size_t max_copy = dst_size - 1;
#if defined(__GNUC__) || defined(__clang__)
  size_t src_object_size = __builtin_object_size(src, 1);
  if ((src_object_size != (size_t)-1) && (src_object_size > 0) &&
      ((src_object_size - 1) < max_copy))
    max_copy = src_object_size - 1;
#endif
  size_t copy_len = strnlen(src, max_copy);
  memcpy(dst, src, copy_len);
  dst[copy_len] = '\0';
}

static inline int dacp_tlv_crawl_checked(char **cursor, ssize_t *remaining, uint32_t *type,
                                         int32_t *length) {
  if ((cursor == NULL) || (*cursor == NULL) || (remaining == NULL) || (type == NULL) ||
      (length == NULL))
    return -1;

  if (*remaining < 8)
    return -1;

  uint32_t raw_type;
  uint32_t raw_length;
  memcpy(&raw_type, *cursor, sizeof(raw_type));
  memcpy(&raw_length, *cursor + 4, sizeof(raw_length));

  uint32_t payload_length = ntohl(raw_length);
  ssize_t needed = 8 + (ssize_t)payload_length;
  if (needed < 8 || needed > *remaining)
    return -1;

  *type = ntohl(raw_type);
  *length = (int32_t)payload_length;
  *cursor += needed;
  *remaining -= needed;
  return 0;
}

static inline int dacp_monitor_transition_to_stopped(int *monitor_initialised) {
  if (monitor_initialised == NULL)
    return 0;
  if (*monitor_initialised == 0)
    return 0;
  *monitor_initialised = 0;
  return 1;
}
