/** @file server.c
 *  @brief Definition of the setup() function.
 * 
 *  This contains setup() function definition that is used for
 *  setting the server up.
 * 
 *  @author Valentine Y. Shevelev (Rokoel)
 *  @bug No known bugs.
 */

#include "helpful.h"
#include "server.h"
#include "socket.h"

#include <unistd.h>
#include <stdio.h>    /*  */
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <fcntl.h>    /* fseek */

/** @brief Sets up the server socket.
 * 
 *  This function opens socket, allows its reuse, sets it to non-blocking,
 *  initializes related Server structure, binds the socket and prepares to listen
 *  for inctoming connections.
 *  
 *  @param serv Server structure to be filled.
 *  @param port Port number that was set by the choose_port() function.
 *  @param maximum_of_connections Maximum of accepted connection.
 *  @param logs A pointer to logs' file.
 *  @return Port that is now used for the server.
 */
int setup(struct Server *serv, int port, int maximum_of_connections, FILE *logs){

    /* Tries to open a socket. */
    (*serv).server_file_descriptor = socket(AF_INET, SOCK_STREAM, 0);
    if ((*serv).server_file_descriptor < 0) {
        fprintf(logs, "ERROR in %s: An error occured when opening socket.", __func__);
        fclose(logs);
        perror("ERROR opening socket.");
        exit(1);
    }

    /* Allows to reuse sockets (solves a bug "Adress already in use!"). */
    int optval = 1;
    if (setsockopt((*serv).server_file_descriptor, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int)) < 0){
        fprintf(logs, "ERROR in %s: An error occured when executing setsockopt() function.", __func__);
        fclose(logs);
        perror("Error on setsockopt().");
        exit(1);
    }
    
    /* Tries to get flags on TCP socket. */
    int flags = fcntl((*serv).server_file_descriptor, F_GETFL);
    if(flags == -1){
        fprintf(logs, "ERROR in %s: An error occured when getting flags on TCP listening socket.", __func__);
        fclose(logs);
        perror("Error on getting flags on TCP listening socket.");
        exit(1);
    }

    /* Tries to set the socket to be non-blocking. */
    if (fcntl((*serv).server_file_descriptor, F_SETFL, flags | O_NONBLOCK) == -1){
        fprintf(logs, "ERROR in %s: An error occured when setting TCP listening socket to be non-blocking.", __func__);
        fclose(logs);
        perror("Error on setting TCP listening socket to be non-blocking.");
        exit(1);
    }
    
    /* Initializes Server structure. */
    bzero((char *) &((*serv).address), sizeof((*serv).address));
    (*serv).port = port;
    (*serv).address.sin_family = AF_INET;
    (*serv).address.sin_addr.s_addr = INADDR_ANY;
    (*serv).address.sin_port = htons((*serv).port);

    /* Tries to bind socket. */
    if (bind((*serv).server_file_descriptor, (struct sockaddr *) &((*serv).address), sizeof((*serv).address)) < 0) {
        fprintf(logs, "ERROR in %s: An error occured when binding server socket.", __func__);
        perror("ERROR on binding.");
        fclose(logs);
        exit(1);
    }

    /* Starts listening for incoming connections. */
    listen((*serv).server_file_descriptor, maximum_of_connections);
    fprintf(logs, "%s: Started listening for inconing connections.\n", __func__);

    return port;
}