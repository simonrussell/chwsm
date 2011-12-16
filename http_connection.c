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
  free(http); 
}

static
int quick_write(int fd, const char *string)
{
  return write(fd, string, strlen(string));
}

void write_response(http_connection *http)
{
  quick_write(http->watcher.fd, "HTTP/1.0 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 11\r\n");
  
  if (http_should_keep_alive(&http->parser))
  {
    quick_write(http->watcher.fd, "Connection: keep-alive\r\n");
  }
  
  quick_write(http->watcher.fd, "\r\nHello world");
  
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

char read_buffer[8192];

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
}

int http_id = 0;

http_connection *new_http_connection(int socket)
{
  http_connection *result = malloc(sizeof(http_connection));
  
  ev_io_init(&result->watcher, http_callback, socket, EV_READ);
  result->id = ++http_id;
  result->message_complete = 0;  
  http_parser_init(&result->parser, HTTP_REQUEST);
  
  set_nonblock(socket);
  ev_io_start(event_loop, &result->watcher);

  return result;
}

int message_complete_callback(http_parser *parser)
{
  http_connection *http = PARSER_TO_CONNECTION(parser);
  
  http->message_complete = 1;
  
  write_response(http);
  
  return 0;
}

int url_callback(http_parser *parser, const char *at, size_t length)
{
  UNUSED(parser);
  UNUSED(at);
  UNUSED(length);

/*  http_connection *http = PARSER_TO_CONNECTION(parser);
    
  printf("%i: ", http->id);
  fwrite(at, length, 1, stdout);
  puts("");*/
  
  return 0;
}

void setup_http_parser_settings(void)
{
  memset(&http_settings, 0, sizeof(http_settings));
  
  http_settings.on_message_complete = message_complete_callback;
  http_settings.on_url = url_callback;
  http_settings.on_header_field = url_callback;
  http_settings.on_header_value = url_callback;
}