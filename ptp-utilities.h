#ifndef PTP_UTILITIES_H
#define PTP_UTILITIES_H

#include "nqptp-shm-structures.h"
#include <stdint.h>

int ptp_get_clock_info(uint64_t *actual_clock_id, uint64_t *time_of_sample, uint64_t *raw_offset, int64_t *clock_change_offset,
                       uint64_t *mastership_start_time);

// Sends a control message to NQPTP.
// Best-effort: logs failures and returns without terminating the process.
void ptp_send_control_message_string(const char *msg);

int ptp_shm_interface_open();
int ptp_get_clock_version();
int ptp_shm_interface_close();

#endif /* PTP_UTILITIES_H */
