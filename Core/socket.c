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
#define MAX_REQUESTED_ROUTE_SIZE 32

#define STYLES_CSS "css/styles.css"
#define SCRIPTS_JS "js/scripts.js"
#define ICON_ICO   "images/icon.ico"

#define MAX_ERRS 8

void* process(void* sock);
short send_response(int socket, int response_type, char *filename);
short send_content(int socket, char *filename, char *headers, int error_counter);
unsigned long get_file_data(char *filename, char *content_type, int *response_code);
int get_filename(char buffer[], char *route);
int is_it_this_possible_route(char buffer[], char *possible_route);


void work_with_socket(int *server_descriptor){
    
    Socket sock;
    int clilen = 16; // from sizeof() (it is always the same)

    sock.socket_file_descriptor = accept(*server_descriptor, (struct sockaddr *) &(sock.address), &clilen);

    if (sock.socket_file_descriptor < 0) {
        error("ERROR on accept"); // should we just close socket? Or just ignore?
    }

    pthread_t t;
    pthread_create(&t, NULL, process, &(sock.socket_file_descriptor));
}

void* process(void* sock){
    int error_flag = 0, n;
    int socket = *((int*)sock);
    char buffer[BUFSIZE];
    bzero(buffer, BUFSIZE-1);
    while (error_flag < MAX_ERRS){
        n = read(socket, buffer, BUFSIZE-1);
        if(n < 0){
            error_flag++;
        }
        else{
            error_flag = 0;
            break;
        }
    }
    buffer[BUFSIZE-1] = '\0';
    char filename[MAX_FILENAME_SIZE];
    
    if(!error_flag){
        printf("%s\n", buffer);
        strcpy(filename, "");
    }
    else{
        printf("PROCESS: Error occured when reading from socket. I will return 404 Not Found");
        // we just empty our buffer so it will automaticly get 404
        // In the future if it consumes a lot of memory and/or processing power,
        // replace it with just manually returning 404
        bzero(buffer, BUFSIZE-1);
        buffer[BUFSIZE-1] = '\0';
        strcpy(filename, "");
    }

    int response_code = get_filename(buffer, filename);
    printf("PROCESS: Response Code: %d\n", response_code);

    printf("PROCESS: Sending response...\n");
    if(send_response(socket, response_code, filename) == -1) printf("PROCESS: We had errors!\n\n");
    else printf("PROCESS: Response sent successfully!\n\n");

    close(socket);
    return NULL;
}

int get_filename(char buffer[], char *filename){
    short is_index_html = 1;
    int response_code = 200;
    strcat(filename, "../Public/Views/");

    /////////////////////////////// ROUTING //////////////////////////////////////
    if(is_it_this_possible_route(buffer, "") ||
    is_it_this_possible_route(buffer, "/main")){
        strcat(filename, "main/");
    }
    
    else if(is_it_this_possible_route(buffer, "/about")){
        strcat(filename, "about/");
    }

    else{
        strcat(filename, "err404/");
        response_code = 404;
    }

    /////////////////////// FILES FILENAMES //////////////////////////////////////
    if(strstr(buffer, "GET") || response_code == 404){
        if(strstr(buffer, "/styles.css")){
            strcat(filename, STYLES_CSS);
            is_index_html = 0;
            response_code = 200; // this is for err404
        }
        if(strstr(buffer, "/scripts.js")){
            strcat(filename, SCRIPTS_JS);
            is_index_html = 0;
            response_code = 200; // this is for err404
        }
        if(strstr(buffer, "/icon.ico")){
            strcat(filename, ICON_ICO);
            is_index_html = 0;
            response_code = 200; // this is for err404
        }
        if(strstr(buffer, "/beginning.webm") && strstr(filename, "main/")){
            strcat(filename, "videos/beginning.webm");
            is_index_html = 0;
            response_code = 200; // this is for err404
        }
        if(strstr(buffer, "/beginning_img.png") && strstr(filename, "main/")){
            strcat(filename, "images/beginning_img.png");
            is_index_html = 0;
            response_code = 200; // this is for err404
        }
        if(is_index_html == 1){
            strcat(filename, "index.html");
        }
    }

    printf("GET_FILENAME: Filename: %s\n", filename);

    return response_code;
}

int is_it_this_possible_route(char buffer[], char *possible_route){
    char temp_bufer[MAX_REQUESTED_ROUTE_SIZE];

    char html_route[MAX_REQUESTED_ROUTE_SIZE];
    if(!strcmp(possible_route, "")){
        // string is empty => main page
        strcpy(html_route, "GET / ");
    }
    else{
        // example result: "GET /about"
        sprintf(html_route, "GET %s ", possible_route);
    }

    char css_route [MAX_REQUESTED_ROUTE_SIZE] = "GET %s/%s ";
    sprintf(temp_bufer, css_route, possible_route, STYLES_CSS);
    strcpy(css_route, temp_bufer);

    char js_route  [MAX_REQUESTED_ROUTE_SIZE] = "GET %s/%s ";
    sprintf(temp_bufer, js_route, possible_route, SCRIPTS_JS);
    strcpy(js_route, temp_bufer);

    char ico_route [MAX_REQUESTED_ROUTE_SIZE] = "GET %s/%s ";
    sprintf(temp_bufer, ico_route, possible_route, ICON_ICO);
    strcpy(ico_route, temp_bufer);

    char beginning_image_route [MAX_REQUESTED_ROUTE_SIZE] = "GET %s/%s ";
    sprintf(temp_bufer, beginning_image_route, possible_route, "images/beginning_img.png");
    strcpy(beginning_image_route, temp_bufer);

    char video_route [MAX_REQUESTED_ROUTE_SIZE] = "GET %s/%s ";
    sprintf(temp_bufer, video_route, possible_route, "videos/beginning.webm");
    strcpy(video_route, temp_bufer);

    if(strstr(buffer, html_route) != NULL || strstr(buffer, css_route) != NULL ||
    strstr(buffer, js_route) != NULL || strstr(buffer, ico_route) != NULL ||
    strstr(buffer, video_route) != NULL || strstr(buffer, beginning_image_route) != NULL){
        return 1;
    }
    return 0;
}

short send_response(int socket, int response_code, char *filename){
    char current_line[MAX_HEADER_LINE_SIZE];
    strcpy(current_line, "");
    char headers[MAX_HEADERS_SIZE];
    strcpy(headers, "");

    char content_type[MAX_CONTENT_TYPE_SIZE];
    strcpy(content_type, "");
    unsigned long file_size = get_file_data(filename, content_type, &response_code);

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

    strcat(headers, "\n"); // empty line before content
    return send_content(socket, filename, headers, 0);
}

unsigned long get_file_data(char *filename, char *content_type, int *response_code){
    struct stat file_data;
    if (stat(filename, &file_data) < 0) { // that basicly can't happen but it should be replaced for prod
        *response_code = 404;
        strcpy(filename, "../Public/Views/err404/index.html");
        return get_file_data(filename, content_type, response_code);
    }
    if (strstr(filename, ".html"))
        strcpy(content_type, "text/html");
    else if (strstr(filename, ".gif"))
        strcpy(content_type, "image/gif");
    else if (strstr(filename, ".jpg"))
        strcpy(content_type, "image/jpg");
    else if (strstr(filename, ".png"))
        strcpy(content_type, "image/png");
    else if (strstr(filename, ".css"))
        strcpy(content_type, "text/css");
    else if (strstr(filename, ".js"))
        strcpy(content_type, "text/javascript");
    else if (strstr(filename, ".ico"))
        strcpy(content_type, "image/x-icon");
    else if (strstr(filename, ".webm"))
        strcpy(content_type, "video/webm");
    else 
        strcpy(content_type, "text/plain");

    return (unsigned long)file_data.st_size;
}

short send_content(int socket, char *filename, char *headers, int error_counter){
    short err;
    size_t f_size;
    char * f_data;
    f_data = c_read_file(filename, &err, &f_size);
    if (err) {
        if(error_counter < MAX_ERRS){
            // we try a little more
            return send_content(socket, filename, headers, error_counter+1);
        }
        else{
            return -1;
        }
    }
    else {
        int n = write(socket, headers, strlen(headers));
        if(n < 0){
            if(error_counter < MAX_ERRS){
                // we try a little more
                return send_content(socket, filename, headers, error_counter+1);
            }
            else{
                return -1;
            }
        }
        n = write(socket, f_data, f_size);
        if(n < 0){
            if(error_counter < MAX_ERRS){
                // we try a little more
                return send_content(socket, filename, headers, error_counter+1);
            }
            else{
                return -1;
            }
        }
        printf("SEND_FILE: Sended file was %d bytes.\n", n);
        free(f_data);
        return 0;
    }
}
