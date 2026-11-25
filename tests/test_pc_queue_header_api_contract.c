#include "metadata/pc_queue.h"

typedef void (*init_signature)(pc_queue *, char *, size_t, uint32_t, const char *);
typedef void (*delete_signature)(pc_queue *);
typedef int (*add_signature)(pc_queue *, const void *, int);
typedef int (*get_signature)(pc_queue *, void *);

_Static_assert(__builtin_types_compatible_p(__typeof__(&pc_queue_init), init_signature),
               "pc_queue_init signature changed");
_Static_assert(__builtin_types_compatible_p(__typeof__(&pc_queue_delete), delete_signature),
               "pc_queue_delete signature changed");
_Static_assert(__builtin_types_compatible_p(__typeof__(&pc_queue_add_item), add_signature),
               "pc_queue_add_item signature changed");
_Static_assert(__builtin_types_compatible_p(__typeof__(&pc_queue_get_item), get_signature),
               "pc_queue_get_item signature changed");

int main(void) { return 0; }
