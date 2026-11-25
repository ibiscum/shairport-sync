#include "metadata/core.h"

typedef int (*send_metadata_to_queue_sig_t)(pc_queue *queue, const uint32_t type,
                                            const uint32_t code, const char *data,
                                            const uint32_t length, rtsp_message *carrier,
                                            int block);

typedef int (*send_metadata_sig_t)(const uint32_t type, const uint32_t code, const char *data,
                                   const uint32_t length, rtsp_message *carrier, int block);

typedef int (*send_ssnc_metadata_sig_t)(const uint32_t code, const char *data,
                                        const uint32_t length, const int block);

typedef int (*metadata_fourcc_from_string_sig_t)(const char *text, uint32_t *fourcc);

_Static_assert(
    __builtin_types_compatible_p(__typeof__(&send_metadata_to_queue), send_metadata_to_queue_sig_t),
    "send_metadata_to_queue signature drifted from core.h contract");

_Static_assert(__builtin_types_compatible_p(__typeof__(&send_metadata), send_metadata_sig_t),
               "send_metadata signature drifted from core.h contract");

_Static_assert(__builtin_types_compatible_p(__typeof__(&send_ssnc_metadata), send_ssnc_metadata_sig_t),
               "send_ssnc_metadata signature drifted from core.h contract");

_Static_assert(
    __builtin_types_compatible_p(__typeof__(&metadata_fourcc_from_string),
                                 metadata_fourcc_from_string_sig_t),
    "metadata_fourcc_from_string signature drifted from core.h contract");

int main(void) { return 0; }
