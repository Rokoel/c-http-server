#include "helpful.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <unistd.h>

#include <sys/socket.h>
#include <sys/stat.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFSIZE 1024
#define MAX_FILENAME_SIZE 64
#define MAX_HEADER_LINE_SIZE 96
#define MAX_CONTENT_TYPE_SIZE 64
#define MAX_HEADERS_SIZE 256

#define NO_PAGE -1
#define MAIN 0
#define ABOUT 1

void* process(void* sock);
void send_response(int socket, short response_type, char *filename);
void send_file(int socket, char *filename);
unsigned long get_file_data(char *filename, char *content_type);
int get_filename(char buffer[], char *route);


void work_with_socket_socket(int *server_descriptor){
    
    Socket sock;
    int clilen = 16; // from sizeof() (it is always the same)

    sock.socket_file_descriptor = accept(*server_descriptor, (struct sockaddr *) &(sock.address), &clilen);

    if (sock.socket_file_descriptor < 0) {
        error("ERROR on accept");
    }

    pthread_t t;
    pthread_create(&t, NULL, process, &(sock.socket_file_descriptor));
}

void* process(void* sock){
    int socket = *((int*)sock);
    char buffer[BUFSIZE];
    bzero(buffer, BUFSIZE-1);
    int n = read(socket, buffer, BUFSIZE-1);
    if(n < 0) {
        error("ERROR on recv");
    }
    buffer[BUFSIZE-1] = '\0';

    printf("%s\n", buffer);

    char filename[MAX_FILENAME_SIZE];
    strcpy(filename, "");

    short response_code = get_filename(buffer, filename);
    printf("PROCESS: Response Code: %d\n", response_code);

    printf("PROCESS: Sending response...\n");
    send_response(socket, response_code, filename);
    printf("PROCESS: Response sent successfully!\n\n");

    close(socket);
    return NULL;
}

int get_filename(char buffer[], char *filename){
    short what_page = NO_PAGE;
    short is_index_html = 1;
    int response_code = 200;
    strcat(filename, "../Public/Views/");

    /////////////////////////////// ROUTING //////////////////////////////////////
    if(strstr(buffer, "GET / ") != NULL || strstr(buffer, "GET /css/styles.css ") != NULL ||
        strstr(buffer, "GET /js/scripts.js ") != NULL || strstr(buffer, "GET /favicon.ico ")!= NULL ){
        what_page = MAIN;
    }
    
    if(strstr(buffer, "GET /about ") || strstr(buffer, "GET /about/css/styles.css ") ||
        strstr(buffer, "GET /about/js/scripts.js ") || strstr(buffer, "GET /about/favicon.ico ")){
        what_page = ABOUT;
    }
    //////////////////////////////////////////////////////////////////////////////

    switch (what_page)
    {
    case MAIN:
        strcat(filename, "main/");
        break;
    case ABOUT:
        strcat(filename, "about/");
        break;
    default:
        strcat(filename, "err404/");
        response_code = 404;
        break;
    }

    if(strstr(buffer, "GET")){
        if(strstr(buffer, "/styles.css")){
            strcat(filename, "css/styles.css");
            is_index_html = 0;
        }
        if(strstr(buffer, "/scripts.js")){
            strcat(filename, "js/scripts.js");
            is_index_html = 0;
        }
        if(strstr(buffer, "/favicon.ico")){
            strcat(filename, "icon.ico");
            is_index_html = 0;
        }
        if(is_index_html == 1){
            strcat(filename, "index.html");
        }
    }

    printf("GET_FILENAME: Filename: %s\n", filename);

    return response_code;
}

void send_response(int socket, short response_code, char *filename){
    char current_line[MAX_HEADER_LINE_SIZE];
    strcpy(current_line, "");
    char headers[MAX_HEADERS_SIZE];
    strcpy(headers, "");

    char content_type[MAX_CONTENT_TYPE_SIZE];
    strcpy(content_type, "");
    unsigned long file_size = get_file_data(filename, content_type);

    switch (response_code){
        case 200:
        {
            strcat(headers, "HTTP/1.1 200 OK\n");
            break;
        }
        
        case 404:
        {
            strcat(headers, "HTTP/1.1 404 Not Found\n");
            break;
        }

        default:
        {
            // Internal Server Error, so this is unexpected behavior.
            strcat(headers, "HTTP/1.1 500 Internal Server Error\n");
            break;
        }
    }
        strcat(headers, "Server: Rokoel WebServer\n");
        
        sprintf(current_line, "Content-length: %lu\n", file_size);
        strcat(headers, current_line);

        sprintf(current_line, "Content-type: %s\n", content_type);
        strcat(headers, current_line);

        printf("SEND_RESPONSE:\n");
        printf("Response headers:\n");
        printf("-----------------\n");
        printf("%s", headers);
        printf("-----------------\n");

        strcat(headers, "\n\r\n"); // empty line before content
        write(socket, headers, strlen(headers));
        send_file(socket, filename);
}

unsigned long get_file_data(char *filename, char *content_type){
    struct stat file_data;
    if (stat(filename, &file_data) < 0) { // that basicly can't happen but it should be replaced for prod
        error("404 FileNotFound Error");
    }
    if (strstr(filename, ".html"))
        strcpy(content_type, "text/html");
    else if (strstr(filename, ".gif"))
        strcpy(content_type, "image/gif");
    else if (strstr(filename, ".jpg"))
        strcpy(content_type, "image/jpg");
    else if (strstr(filename, ".css"))
        strcpy(content_type, "text/css");
    else if (strstr(filename, ".js"))
        strcpy(content_type, "text/javascript");
    else if (strstr(filename, ".ico"))
        strcpy(content_type, "image/x-icon");
    else 
        strcpy(content_type, "text/plain");

    return (unsigned long)file_data.st_size;
}

void send_file(int socket, char *filename){
    short err;
    size_t f_size;
    char * f_data;
    f_data = c_read_file(filename, &err, &f_size);
    if (err) {
        // should we replace it with just closing the socket or..? We actually sended 200 OK to client so may be that's wrong(?)
        // this can't be left for prod because it may turn off the server
        error("Error opening file!");
    }
    else {
        write(socket, f_data, f_size);
        free(f_data);
    }
}
