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