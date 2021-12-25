#ifndef SOCKET_H
#define SOCKET_H

#include <sys/socket.h>
#include <netinet/in.h>

typedef struct Socket{
    int socket_file_descriptor;
    struct sockaddr_in address;
} Socket;

void work_with_socket(int *server_descriptor);

#endif