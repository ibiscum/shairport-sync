#ifndef PTP_UTILITIES_H
#define PTP_UTILITIES_H

#include <stdint.h>

// Returns a clock status code and writes available outputs.
// Any output pointer may be NULL if that value is not needed.
int ptp_get_clock_info(uint64_t *actual_clock_id, uint64_t *time_of_sample, uint64_t *raw_offset,
                       uint64_t *mastership_start_time);

// Sends a control message to NQPTP.
// Best-effort: logs failures and returns without terminating the process.
void ptp_send_control_message_string(const char *msg);

int ptp_shm_interface_open();
int ptp_get_clock_version();
int ptp_shm_interface_close();

#endif /* PTP_UTILITIES_H */
