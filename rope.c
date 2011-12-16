#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rope.h"

typedef struct rope_segment {
  struct rope_segment *next;
  size_t length;
  char data[];
} rope_segment;

rope *rope_create(void)
{
  rope *rope = malloc(sizeof(rope));
  
  rope_init(rope);
  
  return rope;
}

void rope_init(rope *rope)
{
  rope->first = rope->last = NULL;
  rope->length = 0;
}

void rope_destroy(rope *rope)
{
  rope_clear(rope);
  free(rope);
}

void rope_clear(rope *rope)
{
  rope_segment *segment = rope->first;
  while(segment)
  {
    rope_segment *next = segment->next;
    free(segment);
    segment = next;
  }

  rope_init(rope);
}

static
rope_segment *rope_segment_create(rope_segment *next, const char *data, size_t length)
{
  rope_segment *segment = malloc(sizeof(rope_segment) + length + 1);
  
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
  if (length <= 0)
  {
    return;
  }
  
  rope_segment *segment = rope_segment_create(NULL, data, length);
  
  if (!rope->first)
  {
    rope->first = segment;
  }

  if (rope->last)
  {
    rope->last->next = segment;
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
