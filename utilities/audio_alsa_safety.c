#include "audio_alsa_safety.h"
#include <errno.h>

int audio_alsa_validate_strictly_positive_int(int value) { return value > 0; }

int audio_alsa_validate_play_request(const void *buf, int samples) {
  if (samples < 0)
    return -EINVAL;
  if ((samples > 0) && (buf == NULL))
    return -EINVAL;
  return 0;
}

void audio_alsa_reset_channel_map_string(char *map, size_t map_size) {
  if ((map != NULL) && (map_size > 0))
    map[0] = '\0';
}
