/** @file socket.c
 *  @brief File that contains all main cycle functions.
 * 
 *  This file gets all incoming requests and processes them. It contains all these functions:
 *  - accept_socket_and_open_thread, which accepts an incoming connection and creates
 *  a thread for processing it,
 * 
 *  - process_socket, which processes the connection in a thread,
 * 
 *  - send_response, which sends headers and requested files to the client,
 * 
 *  - send_content, which sends requested files to the client,
 * 
 *  - get_file_data, which gets requested file's content-type and responce code,
 * 
 *  - get_filename, which gets file path for the contents of the page that was requested,
 * 
 *  - is_it_this_possible_route, which checks if this is the page that was requested,
 * 
 *  - get_file, which gets only file's filename,
 * 
 *  - check_route, which checks if the string, that is passed as an argument, matches
 *  one of the routes.
 * 
 *  @author Valentine Y. Shevelev (Rokoel)
 *  @bug Server dies if a specific request is received (or memory leaks occured).
 *  @todo Logging system.
 */

#include "helpful.h"
#include "socket.h"
#include "server.h"
#include "split_buffer.h"

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
#define MAX_POSSIBLE_ROUTE_SIZE 34

#define CSS     "css/"
#define JS      "js/"
#define IMAGES  "images/"
#define VIDEOS  "videos/"
#define AUDIOS  "audios/"

#define MAX_ERRS 8

/**
 * @brief Function that waits for connections and accepts them.
 * 
 * This function waits until a connection, receives it and sends it for
 * processing in a new thread with function process().
 * 
 * @param server_descriptor Server socket descriptor that is created in setup().
 * @return Returns void.
 */
void accept_socket_and_open_thread(int *server_descriptor, char *logs_str){

    /* Creates Socket structure. */
    Socket sock;

    /* Accepts an incoming connection. */
    int clilen = 16; /* that is from sizeof() and always stays the same */
    sock.socket_file_descriptor = accept(*server_descriptor, (struct sockaddr *) &(sock.address), &clilen);

    /* If we got an error when accepting new request... */
    if (sock.socket_file_descriptor == -1) {
        /* And if it is due to the fact that we haven't got any incoming requests... */
        if (errno == EWOULDBLOCK) {
            /* Sleeps for 0.5 seconds (this number is gotten experementally) */
            /* and skip to another loop cycle. */
            sleep(0.5);
        }
        /* If this is a different error... */
        else {
            FILE *logs = fopen(logs_str, "a+");
            print(logs, "An error occured when accepting connection!");
            fclose(logs);
        }
    }
    /* If there are no errors... */
    else {
        /* Saves socket file descriptor in newly allocated memory */
        /* for it to exist after compiler reaches end of */
        /* accept_socket_and_open_thread() function. */
        int *pconnfd = malloc(sizeof *pconnfd);
        *pconnfd = sock.socket_file_descriptor;
        /* Opens a new thread. */
        pthread_t t;
        pthread_create(&t, NULL, process_socket, pconnfd);
    }

}

/**
 * @brief Function that in a new thread processes a request.
 * 
 * This function receives data from socket, processes it, sends data back to socket and closes it.
 * It does it in a new thread.
 * 
 * @param sock Connection socket descriptor that is immediately saved inside the function and destroyed.
 * @return Returns void.
*/
void* process_socket(void* sock){
    /* Saves client socket descriptor to local variable so as it can free previousely allocated memory. */
    int socket = *((int*)sock);
    free(sock);
    
    char *current_log_file = (char*)malloc(sizeof(char)*32 + 1), *date_and_time = (char*)malloc(sizeof(char)*DATE_TIME_LEN + 1);
    /* Gets today's date that will be used in naming current log file. */
    format_current_time_and_date(date_and_time, 0);
    /* Sets current_log_file to today's log filename. */
    sprintf(current_log_file, LOGS_LOCATION, date_and_time);
    /* Opens logs file. */
    FILE *logs = fopen(current_log_file, "a+");
    free(current_log_file);
    /* Gets file's filedescriptor. */
    int logs_fd = fileno(logs);
    /* Checks file for being locked/unlocked for writing. */
    struct flock check_file_lock;
    fcntl(logs_fd, F_GETLK, &check_file_lock);
    /* Checks file lock until it is unlocked. */
    while(check_file_lock.l_type != F_UNLCK){
        printf("[%s] in %s(): ", date_and_time, __func__);
        free(date_and_time);
        printf("Logs file is currently locked by another thread. Retrying...\n");
        fcntl(logs_fd, F_GETLK, &check_file_lock);
    }
    /* Fills flock structure for locking the file (because we know it's unlocked now). */
    struct flock set_file_lock;
    set_file_lock.l_type = F_WRLCK;
    set_file_lock.l_whence = SEEK_SET;
    set_file_lock.l_start = 0;
    set_file_lock.l_len = 0;
    set_file_lock.l_pid = getpid();

    /* Locks the file. */
    fcntl(logs_fd, F_SETLK, &set_file_lock);
    
    /* Creating and filling buffer with zeros. */
    char buffer[BUFSIZE];
    bzero(buffer, BUFSIZE-1);
    
    /* Reads from the socket and saves the number of read bytes. */
    int bytes_read;
    bytes_read = read(socket, buffer, BUFSIZE-1);
    
    /* If we haven't received any data yet, we retry until we get data or we've waited for too long. */
    int error_max_counter = 10000, error_counter = 0;
    while(bytes_read == -1 && errno == EWOULDBLOCK && error_counter <= error_max_counter){ // we are reading from socket until it is not blocked
        print(logs, "Hasn't received any data from the socket. Retrying... Received %d bytes.\n", bytes_read);
        bytes_read = read(socket, buffer, BUFSIZE-1);
        ++error_counter;
    }
    /* If an error still remains or if it occured just now, we print about it and close logs, socket and everything. */
    if(bytes_read == -1){
        if(errno != EWOULDBLOCK){
            print(logs, "An unexpected error occured while reading from the socket.");
        }
        else{
            print(logs, "Has been waiting for a responce for too long.");
        }
    }
    /* If everything is fine... */
    else{
        /* Tells how many bytes did it read. */
        print(logs, "Successfully read %d bytes.\n", bytes_read);
        /* "Closes" the buffer and prints it. */
        buffer[BUFSIZE-1] = '\0'; // add end of string to the buffer because read() doesn't do it auto
        print(logs, "Received from client: \n%s\n", buffer);

        print(logs, "Processed into structure:\n")
        struct Request *req = split_buffer(buffer);
        if (req) {
            print(logs, "Method: %d\n", req->method);
            print(logs, "Request-URI: %s\n", req->url);
            print(logs, "HTTP-Version: %s\n", req->version);
            print(logs, "Headers:\n");
            struct Header *h;
            for (h=req->headers; h; h=h->next) {
                print(logs, "%32s: %s\n", h->name, h->value);
            }
            print(logs, "Message-body:\n");
            print(logs, "%s\n", req->body);
        }
        free_request(req);

        /* Creates and initializes filepath. */
        char filepath[MAX_FILENAME_SIZE];
        strcpy(filepath, "");

        /* Gets filepath from the buffer and response code into variables. */
        int response_code = get_filepath(buffer, filepath);
        print(logs, "Response code is %d.\n", response_code);
        
        /* Sends response to the client (headers and requested content). */
        print(logs, "Sending response...\n");
        send_response(socket, response_code, filepath);
        print(logs, "Response sent successfully!\n");
    }
    
    /* Closes everything: socket, logs and thread. */
    close(socket);
    fclose(logs);
    pthread_exit(0);
}

int get_filepath(char buffer[], char *filepath){
    short is_index_html, is_404 = 0;
    int response_code = 200;
    char route[MAX_REQUESTED_ROUTE_SIZE] = "";
    strcat(filepath, "../Public/Views/");

    /////////////////////////////// ROUTING //////////////////////////////////////
    if(is_it_this_possible_route(buffer, "", route)){
        strcat(filepath, "main");
    }
    else if(is_it_this_possible_route(buffer, "main",  route)){
        strcat(filepath, "main");
    }
    else if(is_it_this_possible_route(buffer, "about", route)){
        strcat(filepath, "about");
    }
    else if(is_it_this_possible_route(buffer, "contacts", route)){
        strcat(filepath, "contacts");
    }
    else if(is_it_this_possible_route(buffer, "for_admins", route)){
        strcat(filepath, "for_admins");
    }
    else if(is_it_this_possible_route(buffer, "information", route)){
        strcat(filepath, "information");
    }
    else if(is_it_this_possible_route(buffer, "rules", route)){
        strcat(filepath, "rules");
    }
    else if(is_it_this_possible_route(buffer, "err404", route)){
        response_code = 200;
        strcat(filepath, "err404");
    }
    else {
        response_code = 404;
        strcat(filepath, "err404");
        is_404 = 1;
    }

    strcat(filepath, "/");
    if(!is_404){
        strcat(filepath, route);
    }

    /////////////////////// FILES FILENAMES //////////////////////////////////////
    if(strstr(buffer, "GET") || response_code == 404){
        char filename[MAX_FILENAME_SIZE];
        is_index_html = get_filename(buffer, filename);
        if(is_index_html == 0){
            strcat(filepath, filename);
            response_code = 200;
        }
        if(is_index_html == 1){
            strcat(filepath, "index.html");
        }
        if(is_index_html == -1){
            // placeholder for now, should replace it with Internal Server Error filename
            strcat(filepath, "index.html");
            response_code = 500;
        }
    }

    printf("GET_FILENAME: Filename: %s\n", filepath);
    printf("GET_FILENAME: response_code: %d\n", response_code);
    return response_code;
}

int get_filename(char buffer[], char *filename){
    if ( !strchr(buffer, '\n') ){
        // if buffer has wrong formatting
        strcpy(filename, "");
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
                strcpy(filename, "");
                return 1;
            }
            // /text1/text2/.../image.jpg
            file_str_tok = strtok (url, "/");
            while (file_str_tok != NULL){
                strcpy(filename, file_str_tok);
                file_str_tok = strtok (NULL, "/");
            }
            if(strlen(filename) >= 64){
                strcpy(filename, "");
                printf("GET_FILE: filename is too big. File will be index.html\n");
                return 1;
            }
            printf("FILE: %s\n", filename);
            return 0;
        }
        else{
            // error occured
            printf("FATAL ERROR IN SSCANF in get_file()\n\n");
        }
        // if buffer has wrong formatting
        strcpy(filename, "");
        return -1;
    }
    strcpy(filename, "");
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
