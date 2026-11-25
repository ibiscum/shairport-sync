#pragma once

#include <stddef.h>

// Park a NUL at a line ending and return the next-line pointer.
// Accepts CR, LF, or CRLF endings. Returns NULL if no line ending is found.
char *rtsp_nextline(char *in, int inbuf);
