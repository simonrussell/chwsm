#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <ev.h>
#include "http_connection.h"

#define STRINGIZE_DETAIL(x) #x
#define STRINGIZE(x) STRINGIZE_DETAIL(x)

#define DIE_IF(x) ((x) ? (perror(__FILE__ ":" STRINGIZE(__LINE__)), exit(1)) : 0)
#define DIE_LZ(x) DIE_IF((x) < 0)

#define TRACE printf(__FILE__ ":" STRINGIZE(__LINE__) "\n");

#define UNUSED(x) (void)(x)

struct ev_loop *event_loop;

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

int setup_listener(int port)
{
  int listener = socket(AF_INET, SOCK_STREAM, 0);
  DIE_LZ(listener);

  set_nonblock(listener);
  
  DIE_LZ(bind_socket(listener, port));
  DIE_LZ(listen(listener, 100));
  
  return listener;
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

int main(void)
{
  setup_http_parser_settings();
  
  signal(SIGPIPE, SIG_IGN);
  
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
