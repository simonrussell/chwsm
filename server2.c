#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

typedef int socket_t;

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

int main(void)
{
  socket_t listener = socket(AF_INET, SOCK_STREAM, 0);
  DIE_LZ(listener);
  
  DIE_LZ(bind_socket(listener, 9000));
  DIE_LZ(listen(listener, 0));

  socket_t connection = accept(listener, NULL, 0);
  DIE_LZ(connection);  
 
  write(connection, "Hello World\n", 12); 
 
  DIE_LZ(shutdown(connection, SHUT_RDWR));
  DIE_LZ(close(connection));
  
  DIE_LZ(shutdown(listener, SHUT_RDWR));
  DIE_LZ(close(listener));

  return 0;
}
