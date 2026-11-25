#include "rtsp_line_utilities.h"

char *rtsp_nextline(char *in, int inbuf) {
  char *out = NULL;

  while (inbuf > 0) {
    if (*in == '\r') {
      *in++ = 0;
      inbuf--;
      out = in;
    }

    if ((inbuf > 0) && (*in == '\n')) {
      *in++ = 0;
      inbuf--;
      out = in;
    }

    if (out != NULL)
      break;

    in++;
    inbuf--;
  }

  return out;
}
