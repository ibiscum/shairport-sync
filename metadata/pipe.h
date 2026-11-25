#pragma once

#include "rtsp.h"
#include <inttypes.h>

void metadata_pipe_queue_init(void);
void metadata_pipe_queue_stop(void);
int send_metadata_to_pipe_queue(const uint32_t type, const uint32_t code, const char *data,
                                const uint32_t length, rtsp_message *carrier, int block);