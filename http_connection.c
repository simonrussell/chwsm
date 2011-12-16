#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>

#include "http_connection.h"

#define PARSER_TO_CONNECTION(parser) (http_connection *)((void*)(parser) - offsetof(http_connection, parser))
#define WATCHER_TO_CONNECTION(w) (http_connection *)((void*)(w) - offsetof(http_connection, watcher))

#define UNUSED(x) (void)(x)

extern struct ev_loop *event_loop;

http_parser_settings http_settings;

void set_nonblock(int socket)
{
  fcntl(socket, F_SETFL, fcntl(socket, F_GETFL, 0) | O_NONBLOCK);
}

void http_destroy(http_connection *http)
{
  close(http->watcher.fd);
  ev_io_stop(event_loop, &http->watcher);

  rope_clear(&http->data);
  free(http); 
}

const char *normal_response = "HTTP/1.0 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 11\r\n\r\nHello world";
const char *keepalive_response = "HTTP/1.0 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 11\r\nConnection: keep-alive\r\n\r\nHello world";

static
void setup_response(http_connection *http)
{
  if (http_should_keep_alive(&http->parser))
  {
    http->response = keepalive_response;
  }
  else
  {  
    http->response = normal_response;
  }
  
  http->response_length = strlen(http->response);
}

static
void write_response(http_connection *http)
{
  int bytes_written = write(http->watcher.fd, http->response, http->response_length);

  http->response += bytes_written;
  http->response_length -= bytes_written;
  
  if (http->response_length == 0)
  {
    if (!http_should_keep_alive(&http->parser))
    {
      shutdown(http->watcher.fd, SHUT_RDWR);
      http_destroy(http);
    }
    else
    {
      http->message_complete = 0;
    }
  }
  else
  {
    printf("partial write\n");
  }
}

char read_buffer[8192];

static
void http_callback(EV_P_ ev_io *w, int revents)
{
  UNUSED(loop);
  
  http_connection *http = WATCHER_TO_CONNECTION(w);
  
  if (!http->message_complete && (revents & EV_READ))
  {
    int length = read(http->watcher.fd, read_buffer, sizeof(read_buffer));
    
    if (length > 0)
    {
      int bytes_parsed = http_parser_execute(&http->parser, &http_settings, read_buffer, length);
      
      if (bytes_parsed < length)
      {
        printf("%i: parse error!\n", http->id);
        http_destroy(http);
        return;
      }
    }
    else if ((length < 0 && errno != EWOULDBLOCK) || length == 0)
    {
      http_destroy(http);
      return;
    }
  }
  
  if (http->response_length && (revents & EV_WRITE))
  {
    write_response(http);
  }
}

int http_id = 0;

http_connection *new_http_connection(int socket)
{
  http_connection *result = malloc(sizeof(http_connection));
  
  result->id = ++http_id;
  result->message_complete = 0;  
  result->response = NULL;
  result->response_length = 0;
  http_parser_init(&result->parser, HTTP_REQUEST);
  rope_init(&result->data);
  
  set_nonblock(socket);
  ev_io_init(&result->watcher, http_callback, socket, EV_READ | EV_WRITE);
  ev_io_start(event_loop, &result->watcher);

  return result;
}

static
int message_begin_callback(http_parser *parser)
{
  http_connection *http = PARSER_TO_CONNECTION(parser);

  rope_clear(&http->data);
  
  return 0;  
}

static
int message_complete_callback(http_parser *parser)
{
  http_connection *http = PARSER_TO_CONNECTION(parser);
  
  http->message_complete = 1;
  
//  rope_puts(&http->data);
  setup_response(http);
  
  return 0;
}

#define URL_COLOUR 0
#define HEADER_FIELD_COLOUR 1
#define HEADER_VALUE_COLOUR 2

static
int url_callback(http_parser *parser, const char *at, size_t length)
{
  http_connection *http = PARSER_TO_CONNECTION(parser);
  rope_append_raw(&http->data, URL_COLOUR, at, length);
  return 0;
}

static
int header_field_callback(http_parser *parser, const char *at, size_t length)
{
  http_connection *http = PARSER_TO_CONNECTION(parser);
  rope_append_raw(&http->data, HEADER_FIELD_COLOUR, at, length);
  return 0;
}

static
int header_value_callback(http_parser *parser, const char *at, size_t length)
{
  http_connection *http = PARSER_TO_CONNECTION(parser);
  rope_append_raw(&http->data, HEADER_VALUE_COLOUR, at, length);
  return 0;
}

void setup_http_parser_settings(void)
{
  memset(&http_settings, 0, sizeof(http_settings));
  
  http_settings.on_message_begin = message_begin_callback;
  http_settings.on_message_complete = message_complete_callback;
  http_settings.on_url = url_callback;
  http_settings.on_header_field = header_field_callback;
  http_settings.on_header_value = header_value_callback;
}
