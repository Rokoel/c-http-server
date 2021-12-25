#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

typedef struct Server{
    int server_file_descriptor;
    int port;
    struct sockaddr_in address;
    char buffer[256];
} Server;

int setup(Server *serv, int port, int maximum_of_connections, FILE *logs);

#endif