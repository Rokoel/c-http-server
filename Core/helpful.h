#ifndef HELPFUL_H
#define HELPFUL_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>

typedef struct Server{
    int server_file_descriptor;
    int port;
    struct sockaddr_in address;
    char buffer[256];
} Server;
int setup(Server *serv, int port, int maximum_of_connections);

typedef struct Socket{
    int socket_file_descriptor;
    struct sockaddr_in address;
} Socket;
void work_with_socket_socket(int *server_descriptor);

void error(char *msg);

int choose_port(int argc, char **argv);

char* c_read_file(const char * f_name, short * err, size_t * f_size);

#endif