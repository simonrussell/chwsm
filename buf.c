#include "buf.h"

#define BREATHING_ROOM 1

void buf_init(buffer *buf, char *space, size_t size)
{
  buf->start = buf->current = space;
  buf->end = space + size - BREATHING_ROOM;
}

int buf_append(buffer *buf, char *data, size_t length)
{
  if (buf->current >= buf->end)
  {
    return 0;
  }
  else if (buf->current + length >= buf->end)
  {
    length = buf->end - buf->current;
  }
  
  memcpy(buf->current, data, length);
  buf->current += length;
  *buf->current = '\0';   // just in case.
  
  return length;
}


