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

struct ev_loop *event_loop;

typedef struct {
  ev_io watcher; // must be first!
  int id;
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

void http_callback(EV_P_ ev_io *w, int revents)
{
  http_connection *http = (http_connection *) w;
  
  if (revents | EV_READ)
  {
    char buffer[1024];
    
    int length = read(http->watcher.fd, buffer, 1024);
    
    if (length > 0)
    {
      printf("%i: read %i bytes\n", http->id, length);
      buffer[length] = '\0';
      puts(buffer);
    }
  }
  
  if (revents | EV_WRITE)
  {
    printf("%i: writing!\n", http->id);
    
    char message[] = "HTTP/1.0 200 OK\n\nHello world";
    int result = write(http->watcher.fd, message, sizeof(message)); 
    printf("%i: wrote %i bytes\n", http->id, result);
    
    shutdown(http->watcher.fd, SHUT_RDWR);
    close(http->watcher.fd);
    
    ev_io_stop(event_loop, &http->watcher);
    free(http);
  }
}

int http_id = 0;

http_connection *new_http_connection(int socket)
{
  http_connection *result = malloc(sizeof(http_connection));
  
  result->id = ++http_id;
  
  set_nonblock(socket);
  ev_io_init(&result->watcher, http_callback, socket, EV_READ | EV_WRITE);
  ev_io_start(event_loop, &result->watcher);
  
  return result;
}

void listener_callback(EV_P_ ev_io *w, int revents)
{
  int connection = accept(w->fd, NULL, 0);

  if (connection < 0 && errno == EWOULDBLOCK)
    return;
    
  DIE_LZ(connection);  
 
  http_connection *http = new_http_connection(connection);
  
  printf("%i: accepted!\n", http->id);
}


int main(void)
{
  int listener = setup_listener(9000);
  event_loop = EV_DEFAULT;

  ev_io listener_watcher;
  ev_io_init(&listener_watcher, listener_callback, listener, EV_READ);
  ev_io_start(event_loop, &listener_watcher);
  
  ev_run(event_loop, 0);

  puts("finishing loop");

  DIE_LZ(shutdown(listener, SHUT_RDWR));
  DIE_LZ(close(listener));

  return 0;
}
