#include "metadata/hub.h"

typedef int (*update_string_record_with_data_sig_t)(char **str, const char *data, size_t length);
typedef int (*update_string_record_sig_t)(char **str, const char *s);
typedef int (*update_uint64_record_sig_t)(uint64_record_t *record, const uint64_t value);
typedef void (*metadata_hub_modify_prolog_sig_t)(const char *filename, const int linenumber);
typedef void (*metadata_hub_modify_epilog_sig_t)(int modified, const char *filename,
                                                 const int linenumber);
typedef void (*metadata_hub_read_prolog_sig_t)(const char *filename, const int linenumber);
typedef void (*metadata_hub_read_epilog_sig_t)(const char *filename, const int linenumber);
typedef void (*metadata_hub_queue_init_sig_t)(void);
typedef void (*metadata_hub_queue_stop_sig_t)(void);

_Static_assert(__builtin_types_compatible_p(__typeof__(&update_string_record_with_data),
                                            update_string_record_with_data_sig_t),
               "update_string_record_with_data signature drifted from hub.h contract");

_Static_assert(__builtin_types_compatible_p(__typeof__(&update_string_record),
                                            update_string_record_sig_t),
               "update_string_record signature drifted from hub.h contract");

_Static_assert(__builtin_types_compatible_p(__typeof__(&update_uint64_record),
                                            update_uint64_record_sig_t),
               "update_uint64_record signature drifted from hub.h contract");

_Static_assert(__builtin_types_compatible_p(__typeof__(&_metadata_hub_modify_prolog),
                                            metadata_hub_modify_prolog_sig_t),
               "_metadata_hub_modify_prolog signature drifted from hub.h contract");

_Static_assert(__builtin_types_compatible_p(__typeof__(&_metadata_hub_modify_epilog),
                                            metadata_hub_modify_epilog_sig_t),
               "_metadata_hub_modify_epilog signature drifted from hub.h contract");

_Static_assert(__builtin_types_compatible_p(__typeof__(&_metadata_hub_read_prolog),
                                            metadata_hub_read_prolog_sig_t),
               "_metadata_hub_read_prolog signature drifted from hub.h contract");

_Static_assert(__builtin_types_compatible_p(__typeof__(&_metadata_hub_read_epilog),
                                            metadata_hub_read_epilog_sig_t),
               "_metadata_hub_read_epilog signature drifted from hub.h contract");

_Static_assert(__builtin_types_compatible_p(__typeof__(&metadata_hub_queue_init),
                                            metadata_hub_queue_init_sig_t),
               "metadata_hub_queue_init signature drifted from hub.h contract");

_Static_assert(__builtin_types_compatible_p(__typeof__(&metadata_hub_queue_stop),
                                            metadata_hub_queue_stop_sig_t),
               "metadata_hub_queue_stop signature drifted from hub.h contract");

int main(void) { return 0; }
