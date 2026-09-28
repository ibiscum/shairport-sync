#ifndef _GENERATE_RANDOM_UUID_H
#define _GENERATE_RANDOM_UUID_H

// Returns a newly allocated UUID string, or NULL on allocation failure.
// Caller owns the returned string and must free it.
char *generate_random_uuid(void);

#endif // _GENERATE_RANDOM_UUID_H
