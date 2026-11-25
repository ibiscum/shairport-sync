#pragma once

#include "common.h"
#include "pc_queue.h"
#include "rtsp.h"
#include <pthread.h>
#include <stdint.h>

#define METADATA_FOURCC(a, b, c, d)                                                     \
  (((uint32_t)(uint8_t)(a) << 24) | ((uint32_t)(uint8_t)(b) << 16) |                    \
   ((uint32_t)(uint8_t)(c) << 8) | (uint32_t)(uint8_t)(d))

static inline int metadata_fourcc_from_string(const char *text, uint32_t *fourcc) {
  if ((text == NULL) || (fourcc == NULL))
    return -1;
  for (unsigned i = 0; i < 4; i++) {
    uint8_t c = (uint8_t)text[i];
    if ((c < 0x20) || (c > 0x7E))
      return -1;
  }
  if (text[4] != '\0')
    return -1;
  *fourcc = METADATA_FOURCC(text[0], text[1], text[2], text[3]);
  return 0;
}

typedef struct {
  uint32_t type;
  uint32_t code;
  char *data;
  uint32_t length;
  rtsp_message *carrier;
} metadata_package;

void metadata_pack_cleanup_function(void *arg);
// After all queue users have stopped, release pending packages and delete the queue.
void metadata_queue_delete(pc_queue *queue);

// initialise and stop metadata fanout subsystems

void metadata_init(void);
void metadata_stop(void);

int send_metadata_to_queue(pc_queue *queue, const uint32_t type, const uint32_t code,
                           const char *data, const uint32_t length, rtsp_message *carrier,
                           int block);

// sends metadata out to the metadata system.

int send_metadata(const uint32_t type, const uint32_t code, const char *data, const uint32_t length,
                  rtsp_message *carrier, int block);

// Send metadata with type 'ssnc' and the given code, data and length.
// If no carrier is provided, payload data is duplicated for queue ownership
// and released by metadata_pack_cleanup_function after consumption.
int send_ssnc_metadata(const uint32_t code, const char *data, const uint32_t length,
                       const int block);