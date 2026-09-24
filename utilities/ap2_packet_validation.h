#pragma once

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

// Returns 1 when an allocation result can be safely dereferenced.
int ap2_allocation_succeeded(const void *pointer);

// Returns 1 when data_len can be represented as a payload size that fits in packet_capacity.
int ap2_calculate_buffered_audio_packet_size(uint16_t data_len, size_t packet_capacity,
                                             size_t *packet_size_out);

// Returns 1 when nread is large enough for AP2 decrypt path fixed overheads.
int ap2_encrypted_packet_length_is_valid(ssize_t nread);
