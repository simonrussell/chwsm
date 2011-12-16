#ifndef __HTTP_CONNECTION_H
#define __HTTP_CONNECTION_H

#include <ev.h>
#include "http_parser.h"
#include "rope.h"

typedef struct {
  ev_io watcher; // must be first!
  int id;
  int message_complete;
  http_parser parser;
  rope data;
  const char *response;
  size_t response_length;
} http_connection;

void set_nonblock(int socket);

http_connection *new_http_connection(int socket);
void http_destroy(http_connection *);

void setup_http_parser_settings(void);

#endif

