#include "audio_pw_safety.h"

#include <errno.h>
#include <limits.h>
#include <stddef.h>

static uint64_t saturating_add_u64(uint64_t a, uint64_t b) {
  if (UINT64_MAX - a < b)
    return UINT64_MAX;
  return a + b;
}

int audio_pw_requires_reconfigure(int32_t current_encoded_output_format,
                                  int32_t requested_encoded_format) {
  return current_encoded_output_format != requested_encoded_format;
}

int32_t audio_pw_next_configured_format(int32_t requested_encoded_format, int connect_result) {
  if (connect_result == 0)
    return requested_encoded_format;
  return 0;
}

int audio_pw_validate_configuration_request(int format_supported, unsigned int sample_rate,
                                            unsigned int channel_count) {
  if (format_supported == 0)
    return 0;
  if ((sample_rate == 0) || (sample_rate > 384000))
    return 0;
  if ((channel_count == 0) || (channel_count > 8))
    return 0;
  return 1;
}

int audio_pw_validate_runtime_handles(const void *loop, const void *stream) {
  if ((loop == NULL) || (stream == NULL))
    return -ENODEV;
  return 0;
}

int audio_pw_validate_connect_result(int connect_result) {
  if (connect_result == 0)
    return 0;
  return connect_result;
}

uint64_t audio_pw_calculate_delay_frames(uint64_t queued_frames, uint64_t buffered_frames,
                                         uint64_t fixed_delay_frames,
                                         uint64_t software_queue_frames,
                                         uint64_t played_since_measurement) {
  uint64_t total_frames = 0;
  total_frames = saturating_add_u64(total_frames, queued_frames);
  total_frames = saturating_add_u64(total_frames, buffered_frames);
  total_frames = saturating_add_u64(total_frames, fixed_delay_frames);
  total_frames = saturating_add_u64(total_frames, software_queue_frames);
  if (played_since_measurement >= total_frames)
    return 0;
  return total_frames - played_since_measurement;
}
