#pragma once

#include <stdint.h>

#include "player.h"

void reset_rtp_test_stubs(void);
void set_mock_absolute_time_ns(uint64_t t);
void set_mock_bind_port_base(uint16_t p);
void set_mock_sendto_response(int should_fail);
void set_mock_ptp_response(int response, uint64_t clock_id, uint64_t time_of_sample,
						   uint64_t raw_offset, uint64_t mastership_start_time);

extern int sendto_call_count;
extern int player_put_packet_call_count;
extern seq_t player_put_packet_last_seq;
extern uint32_t player_put_packet_last_timestamp;
extern size_t player_put_packet_last_len;
