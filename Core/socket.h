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

short send_response(int socket, int response_type, char *filename);

short send_content(int socket, char *filename, char *headers, int error_counter);

unsigned long get_file_data(char *filename, char *content_type, int *response_code);

int get_filepath(char buffer[], char *filepath);

int is_it_this_possible_route(char buffer[], char *possible_route, char *actual_route);

int get_filename(char buffer[], char *filename);

int check_route(char buffer[], char route_to_check[], char* possible_route, char *actual_route);

#endif