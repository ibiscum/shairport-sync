#pragma once

#include <stdint.h>

int audio_pw_requires_reconfigure(int32_t current_encoded_output_format,
                                  int32_t requested_encoded_format);

int32_t audio_pw_next_configured_format(int32_t requested_encoded_format, int connect_result);

int audio_pw_validate_configuration_request(int format_supported, unsigned int sample_rate,
                                            unsigned int channel_count);

int audio_pw_validate_runtime_handles(const void *loop, const void *stream);

int audio_pw_validate_connect_result(int connect_result);

uint64_t audio_pw_calculate_delay_frames(uint64_t queued_frames, uint64_t buffered_frames,
                                         uint64_t fixed_delay_frames,
                                         uint64_t software_queue_frames,
                                         uint64_t played_since_measurement);
