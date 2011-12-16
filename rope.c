#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rope.h"

typedef struct rope_segment {
  struct rope_segment *prev, *next;
  size_t length;
  char data[];
} rope_segment;

rope *rope_create(void)
{
  rope *rope = malloc(sizeof(rope));
  
  rope->first = rope->last = NULL;
  rope->length = 0;
  
  return rope;
}

void rope_destroy(rope *rope)
{
  for (rope_segment *segment = rope->first; segment; segment = segment->next)
    free(segment);

  free(rope);
}

static
rope_segment *rope_segment_create(rope_segment *prev, rope_segment *next, const char *data, size_t length)
{
  rope_segment *segment = malloc(sizeof(rope_segment) + length + 1);
  
  segment->prev = prev;
  segment->next = next;
  memcpy(segment->data, data, length);
  segment->data[length] = '\0';
  
  return segment;
}

void rope_append(rope *rope, const char *data)
{
  rope_append_raw(rope, data, strlen(data));
}

void rope_append_raw(rope *rope, const char *data, size_t length)
{
  rope_segment *segment = rope_segment_create(rope->last, NULL, data, length);
  
  if (!rope->first)
  {
    rope->first = segment;
  }

  rope->last = segment;  
  rope->length += length;
}

void rope_puts(rope *rope)
{
  for (rope_segment *segment = rope->first; segment; segment = segment->next)
    fwrite(segment->data, segment->length, 1, stdout);
    
  fwrite("\n", 1, 1, stdout);
}
