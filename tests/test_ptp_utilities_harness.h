#pragma once

#ifndef _COMMON_H
#define _COMMON_H
#endif

#include <stdint.h>

#include "nqptp-shm-structures.h"

typedef struct {
  char *nqptp_shared_memory_interface_name;
} shairport_cfg;

typedef enum {
  clock_ok,
  clock_service_unavailable,
  clock_access_error,
  clock_data_unavailable,
  clock_no_master,
  clock_version_mismatch,
  clock_not_synchronised,
  clock_not_valid,
  clock_no_anchor_info
} clock_status_t;

extern shairport_cfg config;
