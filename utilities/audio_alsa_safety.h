#ifndef AUDIO_ALSA_SAFETY_H
#define AUDIO_ALSA_SAFETY_H

#include <stddef.h>

int audio_alsa_validate_strictly_positive_int(int value);
int audio_alsa_validate_play_request(const void *buf, int samples);
void audio_alsa_reset_channel_map_string(char *map, size_t map_size);

#endif
