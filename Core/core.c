#include "helpful.h"

#include <stdio.h>
#include <signal.h>

#define MAXCONN 100

Server serv;

int main(int argc, char **argv) {
  // signal(SIGPIPE, SIG_IGN); // for not doing anything when write or read breaks
  // signal(SIGABRT, SIG_IGN);
  // signal(SIGSEGV, sighandler); okay ignoring is not the best idea, we need to think of a workaround here when it will be needed. It works for now though.
  int port = setup(&serv, choose_port(argc, argv), MAXCONN);
  printf("MAIN: Server started on port %d\n", port);
  printf("MAIN: Waiting for connetions...\n\n");
  
  while (1){
    work_with_socket(&(serv.server_file_descriptor));
  }
}