#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <ev.h>

#define STRINGIZE_DETAIL(x) #x
#define STRINGIZE(x) STRINGIZE_DETAIL(x)

#define DIE_IF(x) ((x) ? (perror(__FILE__ ":" STRINGIZE(__LINE__)), exit(1)) : 0)
#define DIE_LZ(x) DIE_IF((x) < 0)

int bind_socket(int socket, uint16_t port)
{
  struct sockaddr_in address = {
    .sin_family = AF_INET,
    .sin_port = htons(port),
    .sin_addr = {
      .s_addr = 0
    }
  };
  
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

void listener_callback(EV_P_ ev_io *w, int revents)
{
  int connection = accept(w->fd, NULL, 0);

  if (connection < 0 && errno == EWOULDBLOCK)
    return;
    
  DIE_LZ(connection);  
 
  set_nonblock(connection);
  
  puts("accepted!");

  char message[] = "HTTP/1.0 200 OK\n\nHello world";
  write(connection, message, sizeof(message)); 
 
//    DIE_LZ(shutdown(connection, SHUT_WR));
  DIE_LZ(close(connection));
}

int main(void)
{
  int listener = setup_listener(9000);
  struct ev_loop *loop = EV_DEFAULT;

  ev_io listener_watcher;
  ev_io_init(&listener_watcher, listener_callback, listener, EV_READ);
  ev_io_start(loop, &listener_watcher);
  
  ev_run(loop, 0);

  puts("finishing loop");

  DIE_LZ(shutdown(listener, SHUT_RDWR));
  DIE_LZ(close(listener));

  return 0;
}
