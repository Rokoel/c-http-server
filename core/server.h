#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>    /* struct FILE */
#include <netinet/in.h>    /* struct sockaddr_in */


/** @brief A structure that contains all information about the server socket. */
typedef struct Server{
    int server_file_descriptor;
    int port;
    struct sockaddr_in address;
    char buffer[256];
} Server;

int setup(Server *serv, int port, int maximum_of_connections, FILE *logs);

#endif