#pragma once

// Returns a newly allocated UUID string for a device id, or NULL on failure.
// Caller owns the returned string and must free it.
char *generate_device_uuid(const char *device_id);