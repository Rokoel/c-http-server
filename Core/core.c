#include "helpful.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>

#define MAXERRS 16
#define MAXCONN 100

Server serv;

int main(int argc, char **argv) {
  int port = setup(&serv, choose_port(argc, argv), MAXCONN);
  printf("MAIN: Server started on port %d\n", port);
  printf("MAIN: Waiting for connetions...\n\n");
  
  while (1){
    work_with_socket(&(serv.server_file_descriptor));
  }
}