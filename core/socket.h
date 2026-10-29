#ifndef SOCKET_H
#define SOCKET_H

#include <netinet/in.h>    /* struct sockaddr_in */

/** @brief A structure that contains all information about the client socket. */
typedef struct Socket{
    int socket_file_descriptor;
    struct sockaddr_in address;
} Socket;

void accept_socket_and_open_thread(int *server_descriptor, char *logs_str);

void* process_socket(void* sock);

void send_response(int socket, int response_type, char *filepath, FILE *logs);

void send_content(int socket, char *filepath, char *headers, FILE *logs);

unsigned long get_file_data(char *filepath, char *content_type, int *response_code);

#endif