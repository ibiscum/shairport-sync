#include "ap2_packet_validation.h"

int ap2_allocation_succeeded(const void *pointer) { return pointer != NULL; }

int ap2_calculate_buffered_audio_packet_size(uint16_t data_len, size_t packet_capacity,
                                             size_t *packet_size_out) {
  if (data_len < 2)
    return 0;

  size_t packet_size = (size_t)data_len - 2;
  if (packet_size > packet_capacity)
    return 0;

  if (packet_size_out != NULL)
    *packet_size_out = packet_size;

  return 1;
}

int ap2_encrypted_packet_length_is_valid(ssize_t nread) {
  // 12-byte RTP-like header + 16-byte authentication tag + 8-byte nonce trailer.
  const ssize_t minimum_packet_size = 12 + 16 + 8;
  return nread >= minimum_packet_size;
}
