#pragma once
#include "player.h"

/* Reset meter state.
 * Returns non-zero if there was accumulated data before reset. */
int audio_bitrate_reset(audio_bitrate_meter *meter);
/* Add one compressed packet's contribution.
 * Returns a bitmask of AUDIO_BITRATE_REPORT and/or AUDIO_BITRATE_RESET. */
int audio_bitrate_add(audio_bitrate_meter *meter, uint64_t bytes, uint32_t samples, uint32_t rate,
                      uint32_t sequence, uint32_t timestamp);
/* Emit receiver packet counters as ssnc/arst when due. */
void report_player_counters(rtsp_conn_info *conn);
/* Return the generation token to be used for later accounting.
 * A generation change means previously in-flight accounting must be discarded. */
uint64_t input_bitrate_packet_generation(rtsp_conn_info *conn, uint32_t ssrc, uint32_t sequence,
                                         uint32_t timestamp, int32_t timestamp_gap);
/* Account one packet if its generation token is current and playback is active. */
void account_input_bitrate(rtsp_conn_info *conn, uint32_t ssrc, uint64_t bytes, uint32_t samples,
                           uint32_t sequence, uint32_t timestamp, uint64_t generation);
