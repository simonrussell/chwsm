#ifndef __BUF_H
#define __BUF_H

#include <stddef.h>

typedef struct {
  char *start;
  char *current;
  char *end;    // actually 1 byte less than end
} buffer;

void buf_init(buffer *buf, char *space, size_t size);

//buffer *buf_create(size_t size);
//void buf_destroy(llbuffer *buf);

/*
  copies data into the buffer and appends a zero byte.  Will leave the last byte of the buffer for a zero.
  returns number of bytes copied.
*/
size_t buf_append(buffer *buf, char *data, size_t length);

void buf_print(buffer *buf);

#endif
