#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rope.h"

typedef struct rope_segment {
  struct rope_segment *next;
  int colour;
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
rope_segment *rope_segment_create(rope_segment *next, int colour, const char *data, size_t length)
{
  rope_segment *segment = malloc(sizeof(rope_segment) + length + 1);
  
  segment->length = length;
  segment->colour = colour;
  segment->next = next;
  memcpy(segment->data, data, length);
  segment->data[length] = '\0';
  
  return segment;
}

void rope_append(rope *rope, int colour, const char *data)
{
  rope_append_raw(rope, colour, data, strlen(data));
}

void rope_append_raw(rope *rope, int colour, const char *data, size_t length)
{
  if (length <= 0)
  {
    return;
  }
  
  rope_segment *segment = rope_segment_create(NULL, colour, data, length);
  
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

void rope_puts(const rope *rope)
{
  for (rope_segment *segment = rope->first; segment; segment = segment->next)
  {
    fprintf(stdout, "\e[%im", segment->colour + 31);
    fwrite(segment->data, segment->length, 1, stdout);
  }

  fprintf(stdout, "\e[0m\n");
}
