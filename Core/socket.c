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

#include <fcntl.h>
#include <errno.h>


#define BUFSIZE 1024
#define MAX_FILENAME_SIZE 64
#define MAX_HEADER_LINE_SIZE 96
#define MAX_CONTENT_TYPE_SIZE 64
#define MAX_HEADERS_SIZE 256
#define MAX_REQUESTED_ROUTE_SIZE 32
#define MAX_POSSIBLE_ROUTE_SIZE 34 // !important - MAX_REQUESTED_ROUTE_SIZE + 2 always

#define CSS     "css/"
#define JS      "js/"
#define IMAGES  "images/"
#define VIDEOS  "videos/"
#define AUDIOS  "audios/"

#define MAX_ERRS 8

void* process(void* sock);
short send_response(int socket, int response_type, char *filename);
short send_content(int socket, char *filename, char *headers, int error_counter);
unsigned long get_file_data(char *filename, char *content_type, int *response_code);
int get_filename(char buffer[], char *route);
int is_it_this_possible_route(char buffer[], char *possible_route, char *actual_route);
int get_file(char buffer[], char *file);
int check_route(char buffer[], char route_to_check[], char* possible_route, char *actual_route);

/*
TODO:
- Refactoring
*/

void work_with_socket(int *server_descriptor){
    Socket sock;
    int clilen = 16; // from sizeof() (it is always the same)

    sock.socket_file_descriptor = accept(*server_descriptor, (struct sockaddr *) &(sock.address), &clilen);

    if (sock.socket_file_descriptor == -1) {
        if (errno == EWOULDBLOCK) {
            sleep(0.5); // 17% cpu usage when 0.5 and 95+% when 0 and <17% when > 0.5
            // affects respponsivness of the site (kind of delay between requests)
            // okay results are waaaay better than this^ but more testing is needed
        } else {
            perror("error when accepting connection");
        }
    }
    else {
        // we better do that or else we were reusing one variable... Anyway, that is a reasonable fix from stack overflow (I don't understand it fully)
        // link to the fix https://stackoverflow.com/questions/30756210/unable-to-receive-data-after-connection-getting-bad-file-descriptor-error
        int *pconnfd = malloc(sizeof *pconnfd);
        if (NULL == pconnfd);
        *pconnfd = sock.socket_file_descriptor;
        pthread_t t;
        pthread_create(&t, NULL, process, pconnfd);
    }
}

void* process(void* sock){
    int error_flag = 0, n;
    int socket = *((int*)sock);
    free(sock);
    char buffer[BUFSIZE];
    bzero(buffer, BUFSIZE-1);
    n = read(socket, buffer, BUFSIZE-1);
    printf("READ: %d\n", n);
    if(n == -1){
        if(errno == EWOULDBLOCK){
            while(n == -1 && errno == EWOULDBLOCK){
                n = read(socket, buffer, BUFSIZE-1);
                printf("READ: %d\n", n);
            }
        }
        else{perror(strerror(errno));}
    }
    if(n == 0){
        close(socket);
        pthread_exit(0);
    }
    printf("SUCCESSFULLY READ: %d\n", n);

    buffer[BUFSIZE-1] = '\0';
    char filename[MAX_FILENAME_SIZE];
    printf("BUFFER: %s\n", buffer);
    strcpy(filename, "");

    int response_code = get_filename(buffer, filename);
    printf("PROCESS: Response Code: %d\n", response_code);

    printf("PROCESS: Sending response...\n");
    if(send_response(socket, response_code, filename) == -1) printf("PROCESS: We had errors!\n\n"); // this was disabled for now
    else printf("PROCESS: Response sent successfully!\n\n");

    close(socket);
    pthread_exit(0);
}

int get_filename(char buffer[], char *filename){
    short is_index_html, is_404 = 0;
    int response_code = 200;
    char route[MAX_REQUESTED_ROUTE_SIZE] = "";
    strcat(filename, "../Public/Views/");

    /////////////////////////////// ROUTING //////////////////////////////////////
    if(is_it_this_possible_route(buffer, "", route)){
        strcat(filename, "main");
    }
    else if(is_it_this_possible_route(buffer, "main",  route)){
        strcat(filename, "main");
    }
    else if(is_it_this_possible_route(buffer, "about", route)){
        strcat(filename, "about");
    }
    else if(is_it_this_possible_route(buffer, "contacts", route)){
        strcat(filename, "contacts");
    }
    else if(is_it_this_possible_route(buffer, "for_admins", route)){
        strcat(filename, "for_admins");
    }
    else if(is_it_this_possible_route(buffer, "information", route)){
        strcat(filename, "information");
    }
    else if(is_it_this_possible_route(buffer, "rules", route)){
        strcat(filename, "rules");
    }
    else if(is_it_this_possible_route(buffer, "privacy_policy", route)){
        strcat(filename, "privacy_policy");
    }
    else if(is_it_this_possible_route(buffer, "err404", route)){
        response_code = 200;
        strcat(filename, "err404");
    }
    else {
        response_code = 404;
        strcat(filename, "err404");
        is_404 = 1;
    }

    strcat(filename, "/");
    if(!is_404){
        strcat(filename, route);
    }

    /////////////////////// FILES FILENAMES //////////////////////////////////////
    if(strstr(buffer, "GET") || response_code == 404){
        char file[MAX_FILENAME_SIZE];
        is_index_html = get_file(buffer, file);
        if(is_index_html == 0){
            strcat(filename, file);
            response_code = 200;
        }
        if(is_index_html == 1){
            strcat(filename, "index.html");
        }
        if(is_index_html == -1){
            // placeholder for now, should replace it with Internal Server Error filename
            strcat(filename, "index.html");
            response_code = 500;
        }
    }

    printf("GET_FILENAME: Filename: %s\n", filename);
    printf("GET_FILENAME: response_code: %d\n", response_code);
    return response_code;
}

int get_file(char buffer[], char *file){
    if ( !strchr(buffer, '\n') ){
        // if buffer has wrong formatting
        strcpy(file, "");
        return -1;
    }
    else {
        char *p_first_line, *file_str_tok;
        char safe_buffer[1024];
        strcpy(safe_buffer, buffer);
        char method[16];  // request method
        char url[256];     // request uri
        char version[16]; // request version

        p_first_line = strtok (safe_buffer, "\n");
        if(sscanf(p_first_line, "%s %s %s\n", method, url, version) == 3){
            // if there is no ".", it is index html or 404
            if ( !strchr(url, '.') ){
                strcpy(file, "");
                return 1;
            }
            // /text1/text2/.../image.jpg
            file_str_tok = strtok (url, "/");
            while (file_str_tok != NULL){
                strcpy(file, file_str_tok);
                file_str_tok = strtok (NULL, "/");
            }
            if(strlen(file) >= 64){
                strcpy(file, "");
                printf("GET_FILE: filename is too big. File will be index.html\n");
                return 1;
            }
            printf("FILE: %s\n", file);
            return 0;
        }
        // if buffer has wrong formatting
        strcpy(file, "");
        return -1;
    }
    strcpy(file, "");
    return 1; // if nothing is found, it is index html or 404 html
}

int is_it_this_possible_route(char buffer[], char *poss_route, char *actual_route){
    char possible_route[MAX_REQUESTED_ROUTE_SIZE] = "";
    sprintf(possible_route, "%c%s", '/', poss_route);
    int is_route = 0;
    printf("POSSIBLE_ROUTE in is_it_this_possible_route: %s\n", possible_route);
    printf("BUFFER in is_it_this_possible_route: %s\n", buffer);

    char html_route[MAX_POSSIBLE_ROUTE_SIZE];
    if(!strcmp(possible_route, "/")) strcpy(html_route, "GET / ");
    else sprintf(html_route, " %s ", possible_route);
    if(strstr(buffer, html_route) != NULL){
        strcpy(actual_route, "");
        is_route = 1;
    }

    is_route += check_route(buffer, CSS, possible_route, actual_route);
    is_route += check_route(buffer, JS, possible_route, actual_route);
    is_route += check_route(buffer, IMAGES, possible_route, actual_route);
    is_route += check_route(buffer, VIDEOS, possible_route, actual_route);
    is_route += check_route(buffer, AUDIOS, possible_route, actual_route);

    printf("ACTUAL_ROUTE in is_it_this_possible_route: %s\n", actual_route);
    return is_route;
}

int check_route(char buffer[], char route_to_check[], char* possible_route, char *actual_route){
    char temp_bufer[MAX_POSSIBLE_ROUTE_SIZE];
    if(!strcmp(possible_route, "/") || !strcmp(possible_route, "/main"))
        sprintf(temp_bufer, " /%s", route_to_check);
    else
        sprintf(temp_bufer, " %s/%s", possible_route, route_to_check);

    if(strstr(buffer, temp_bufer) != NULL){
        sprintf(temp_bufer, "%s", route_to_check);
        strcpy(actual_route, temp_bufer);
        printf("ACTUAL ROUTE: %s\n", actual_route);
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
    if(response_code != 500) {
        return send_content(socket, filename, headers, 0);
    }
    else {
        return -1;
    }
}

unsigned long get_file_data(char *filename, char *content_type, int *response_code){
    struct stat file_data;
    if (stat(filename, &file_data) < 0) { // filename is based by us so it is impossible to use it badly
        *response_code = 404;
        strcpy(filename, "../Public/Views/err404/index.html");
        printf("GET_FILE_DATA -> 404\n");
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
    else if (strstr(filename, ".mp4"))
        strcpy(content_type, "video/mp4");
    else if (strstr(filename, ".mp3"))
        strcpy(content_type, "audio/mp3");
    else if (strstr(filename, ".wav"))
        strcpy(content_type, "audio/wav");
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
        printf("SEND_CONTENT: c_read_file_error\n");
        // what should we do?
    }
    else {
        int n = write(socket, headers, strlen(headers));
        if(n == -1){
            if(errno == EWOULDBLOCK){
                while(n == -1 && errno == EWOULDBLOCK){
                    n = write(socket, headers, strlen(headers));
                    printf("WRITE1: %d\n", n);
                }
            }
            else{
                printf("SEND_CONTENT: 1-st write error\n");
                printf("Write1: %d", n);
                // what should we do?
            }
        }
        n = write(socket, f_data, f_size);
        if(n == -1){
            if(errno == EWOULDBLOCK){
                while(n == -1 && errno == EWOULDBLOCK){
                    n = write(socket, f_data, f_size);
                    printf("WRITE2: %d\n", n);
                }
            }
            else{
                printf("SEND_CONTENT: 2-nd write error\n");
                printf("Write2: %d", n);
                // what should we do?
            }
        }
        printf("SEND_CONTENT: Sended file was %d bytes.\n", n);
        free(f_data);
        return 0;
    }
}
