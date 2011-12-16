#ifndef __ROPE_H
#define __ROPE_H

typedef struct rope_segment rope_segment;

typedef struct {
  rope_segment *first, *last;
  size_t length;
} rope;

void rope_init(rope *);
void rope_clear(rope *);

rope *rope_create(void);
void rope_destroy(rope *);

// copies in the data
void rope_append_raw(rope *, const char *, size_t);
void rope_append(rope *, const char *);

void rope_puts(rope *);

#endif

