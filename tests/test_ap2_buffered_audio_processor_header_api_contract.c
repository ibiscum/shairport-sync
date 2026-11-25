#include "ap2_buffered_audio_processor.h"

#if defined(__GNUC__) || defined(__clang__)
typedef void *(*rtp_buffered_audio_processor_sig_t)(void *arg);

_Static_assert(
    __builtin_types_compatible_p(__typeof__(&rtp_buffered_audio_processor),
                                 rtp_buffered_audio_processor_sig_t),
    "rtp_buffered_audio_processor signature drifted from ap2_buffered_audio_processor.h contract");
#endif

int main(void) { return 0; }
