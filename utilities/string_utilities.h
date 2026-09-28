#pragma once

/* from
 * http://coding.debuntu.org/c-implementing-str_replace-replace-all-occurrences-substring#comment-722
 */
// Returns a newly allocated string, or NULL on failure/invalid input.
// Caller owns the returned string and must free it.
char *str_replace(const char *string, const char *substr, const char *replacement);

// Returns a newly allocated service name string, or NULL on failure.
// Caller owns the returned string and must free it.
char *service_name(const char *raw_service_name);