#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <ev.h>
#include <http_parser.h>

#define STRINGIZE_DETAIL(x) #x
#define STRINGIZE(x) STRINGIZE_DETAIL(x)

#define DIE_IF(x) ((x) ? (perror(__FILE__ ":" STRINGIZE(__LINE__)), exit(1)) : 0)
#define DIE_LZ(x) DIE_IF((x) < 0)

#define TRACE printf(__FILE__ ":" STRINGIZE(__LINE__) "\n");

#define UNUSED(x) (void)(x)

#define PARSER_TO_CONNECTION(parser) (http_connection *)((void*)(parser) - offsetof(http_connection, parser))
#define WATCHER_TO_CONNECTION(w) (http_connection *)((void*)(w) - offsetof(http_connection, watcher))

struct ev_loop *event_loop;
http_parser_settings http_settings;

typedef struct {
  ev_io watcher; // must be first!
  int id;
  int message_complete;
  http_parser parser;
} http_connection;

int bind_socket(int socket, uint16_t port)
{
  struct sockaddr_in address = {
    .sin_family = AF_INET,
    .sin_port = htons(port),
    .sin_addr = {
      .s_addr = 0
    }
  };
  
  int reuse_addr = 1;
  setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr));
  
  return bind(socket, (struct sockaddr *) &address, sizeof(address));
}

void set_nonblock(int socket)
{
  fcntl(socket, F_SETFL, fcntl(socket, F_GETFL, 0) | O_NONBLOCK);
}

int setup_listener(int port)
{
  int listener = socket(AF_INET, SOCK_STREAM, 0);
  DIE_LZ(listener);

  set_nonblock(listener);
  
  DIE_LZ(bind_socket(listener, port));
  DIE_LZ(listen(listener, 100));
  
  return listener;
}

void http_destroy(http_connection *http)
{
  close(http->watcher.fd);
  ev_io_stop(event_loop, &http->watcher);
  free(http); 
}

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

void http_callback(EV_P_ ev_io *w, int revents)
{
  UNUSED(loop);
  
  http_connection *http = WATCHER_TO_CONNECTION(w);
  
  if (!http->message_complete && (revents & EV_READ))
  {
    char buffer[8192];
    
    int length = read(http->watcher.fd, buffer, sizeof(buffer));
    
    if (length > 0)
    {      
      int bytes_parsed = http_parser_execute(&http->parser, &http_settings, buffer, length);
      
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

void listener_callback(EV_P_ ev_io *w, int revents)
{ 
  UNUSED(loop);
  UNUSED(revents);
  
  int connection = accept(w->fd, NULL, 0);
  
  if (connection < 0 && errno == EWOULDBLOCK)
    return;
    
  DIE_LZ(connection);
 
  new_http_connection(connection);
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

int main(void)
{
  setup_http_parser_settings();
  
  int listener = setup_listener(9000);
  event_loop = ev_default_loop(EVBACKEND_EPOLL);

  ev_io listener_watcher;
  ev_io_init(&listener_watcher, listener_callback, listener, EV_READ);
  ev_io_start(event_loop, &listener_watcher);
  
  ev_run(event_loop, 0);

  puts("finishing loop");

  DIE_LZ(shutdown(listener, SHUT_RDWR));
  DIE_LZ(close(listener));

  return 0;
}
