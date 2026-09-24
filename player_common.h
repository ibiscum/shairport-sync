#ifndef _PLAYER_COMMON_H
#define _PLAYER_COMMON_H

#include <limits.h>
#include <stddef.h>
#include <stdint.h>

#include "player.h"

static inline size_t player_safe_uncompressed_bytes_to_copy(
        int packet_length_bytes, unsigned int frames_per_packet,
        unsigned int input_bytes_per_frame) {
    size_t requested;
    size_t max_bytes;

    if (packet_length_bytes <= 0)
        return 0;

    requested = (size_t)packet_length_bytes;

    if ((frames_per_packet != 0) && (input_bytes_per_frame > (SIZE_MAX / frames_per_packet)))
        max_bytes = SIZE_MAX;
    else
        max_bytes = (size_t)frames_per_packet * (size_t)input_bytes_per_frame;

    if (requested > max_bytes)
        return max_bytes;

    return requested;
}

static inline const char *player_stream_category_name(airplay_stream_c category) {
    switch (category) {
    case unspecified_stream_category:
        return "unspecified stream";
    case ptp_stream:
        return "PTP stream";
    case ntp_stream:
        return "NTP stream";
    case remote_control_stream:
        return "Remote Control stream";
    case classic_airplay_stream:
        return "Classic AirPlay stream";
    default:
        return "Unexpected stream code";
    }
}

#endif