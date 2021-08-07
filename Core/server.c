#include "helpful.h"

#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>

int setup(struct Server *serv, int port, int maximum_of_connections){
    // Open a socket
    (*serv).server_file_descriptor = socket(AF_INET, SOCK_STREAM, 0);
    if ((*serv).server_file_descriptor < 0) {
        error("ERROR opening socket");
    }

    // Fixes Address already in use problem (is it safe to use? may it cause more problems?)
    int optval = 1;
    if (setsockopt((*serv).server_file_descriptor, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int)) < 0)
        return -1;
    
    // Initialize socket structure
    bzero((char *) &((*serv).address), sizeof((*serv).address));
    (*serv).port = port;
    (*serv).address.sin_family = AF_INET;
    (*serv).address.sin_addr.s_addr = INADDR_ANY;
    (*serv).address.sin_port = htons((*serv).port);

    // Now bind the host address
    if (bind((*serv).server_file_descriptor, (struct sockaddr *) &((*serv).address), sizeof((*serv).address)) < 0) {
        error("ERROR on binding");
    }

    listen((*serv).server_file_descriptor, maximum_of_connections);

    return port;
}