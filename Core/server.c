/** @file server.c
 *  @brief Definition of the setup() function.
 * 
 *  This contains setup() function definition that is used for
 *  setting the server up.
 * 
 *  @author Valentine Y. Shevelev (Rokoel)
 *  @bug No known bugs.
**/

#include "helpful.h"    /* print macro */
#include "server.h"


#include <unistd.h>    /* stuff for working with sockets */
#include <stdio.h>    /* struct FILE & related functions, perror */
#include <sys/socket.h>    /* stuff for working with sockets */
#include <stdlib.h>    /* exit */
#include <netinet/in.h>    /* stuff for working with sockets */
#include <fcntl.h>    /* fcntl */
#include <string.h>    /* bzero */

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
        print(logs, "ERROR - An error occured when opening socket.\n");
        fclose(logs);
        exit(1);
    }

    /* Allows to reuse sockets (solves a bug "Adress already in use!"). */
    int optval = 1;
    if (setsockopt((*serv).server_file_descriptor, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int)) < 0){
        print(logs, "ERROR - An error occured when executing setsockopt() function.\n");
        fclose(logs);
        exit(1);
    }
    
    /* Tries to get flags on TCP socket. */
    int flags = fcntl((*serv).server_file_descriptor, F_GETFL);
    if(flags == -1){
        print(logs, "ERROR - An error occured when getting flags on TCP listening socket.\n")
        fclose(logs);
        exit(1);
    }

    /* Tries to set the socket to be non-blocking. */
    if (fcntl((*serv).server_file_descriptor, F_SETFL, flags | O_NONBLOCK) == -1){
        print(logs, "ERROR - An error occured when setting TCP listening socket to be non-blocking.\n")
        fclose(logs);
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
        print(logs, "ERROR - An error occured when binding the server socket.\n");
        fclose(logs);
        exit(1);
    }

    /* Starts listening for incoming connections. */
    listen((*serv).server_file_descriptor, maximum_of_connections);
    print(logs, "Started listening for inconing connections.\n")

    return port;
}