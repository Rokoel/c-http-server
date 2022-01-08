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
 *  - get_file_data, which gets requested file's content-type and responce code.
 * 
 *  @author Valentine Y. Shevelev (Rokoel)
 *  @bug No known bugs.
 *  @todo POST requests.
 */

#include "helpful.h"    /* print macro */
#include "socket.h"
#include "split_buffer.h"    /* split_buffer */
#include "settings.h"    /* constants */
#include "get_filepath.h"    /* get_filepath */

#include <stdio.h>    /* struct FILE & related functions*/
#include <stdlib.h>    /* malloc, free */
#include <string.h>    /* strlen, strcat, strstr, strcpy */
#include <pthread.h>    /* pthread_create */

#include <unistd.h>    /* socket-related functions */

#include <sys/stat.h>    /* stat */

#include <fcntl.h>    /* fcntl */
#include <errno.h>    /* errno, EWOULDBLOCK */


#define BUFSIZE 1024
#define MAX_FILENAME_SIZE 64
#define MAX_HEADER_LINE_SIZE 96
#define MAX_CONTENT_TYPE_SIZE 64
#define MAX_HEADERS_SIZE 700
#define MAX_REQUESTED_ROUTE_SIZE 32
#define MAX_POSSIBLE_ROUTE_SIZE 34

#define MAX_ERRS 8

/**
 * @brief Function that waits for connections and accepts them.
 * 
 * This function waits until a connection, receives it and sends it for
 * processing in a new thread with function process().
 * 
 * @param server_descriptor Server socket descriptor that is created in setup().
 * @return Returns void.
**/
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
**/
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
        printf("Logs file is currently locked by another thread. Retrying...\n");
        fcntl(logs_fd, F_GETLK, &check_file_lock);
    }
    free(date_and_time);
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
        /* Tells how many bytes it read. */
        print(logs, "Successfully read %d bytes.\n", bytes_read);
        /* "Closes" the buffer and prints it. */
        buffer[BUFSIZE-1] = '\0';
        buffer[BUFSIZE-2] = '\n';
        buffer[BUFSIZE-3] = '\r';
        buffer[BUFSIZE-4] = '\n';
        
        print(logs, "Received from client: \n%s\n", buffer);

        /* Tries to parse the request into a structure. */
        print(logs, "What was processed into a structure:\n")
        struct Request *req = split_buffer(buffer, logs);

        /* If parsing is successful... */
        if (req) {
            print(logs, "Method: %d\n", req->method);
            print(logs, "Request-URI: %s\n", req->url);
            print(logs, "HTTP-Version: %s\n", req->version);
            print(logs, "Headers:\n");
            print(logs, "Message-body:\n");

            /* Creates and initializes filepath. */
            // char *filepath = malloc(strlen(VIEWS_FOLDER) + strlen(req->url) + strlen(INDEX_HTML_FILENAME) + strlen(MAIN_PAGE_FOLDER_WITH_SLASH) + 1);
            char filepath[1024];
            /* Gets filepath from the buffer and response code into variables. */
            int response_code = get_filepath(req->url, filepath, logs);
            print(logs, "Response code is %d.\n", response_code);
            
            /* Sends response to the client (headers and requested content). */
            print(logs, "Sending response...\n");
            send_response(socket, response_code, filepath, logs);
            //free(filepath);
            print(logs, "Response sent successfully!\n");
            /* We free the Request structure either way. */
            free_request(req, logs);
        }
        /* If parsing is unsuccessful... */
        else{
            /* We print about it and return 404. */
            print(logs, "Request has got wrong formatting. Sending err404...\n");
            char *filepath = malloc(strlen(VIEWS_FOLDER) + strlen("/err404/index.html") + strlen(INDEX_HTML_FILENAME) + strlen(MAIN_PAGE_FOLDER_WITH_SLASH) + 1);

            /* Gets filepath from the buffer and response code into variables. */
            int response_code = get_filepath("/err404/index.html", filepath, logs);
            print(logs, "Response code is %d.\n", response_code);
            
            /* Sends response to the client (headers and requested content). */
            print(logs, "Sending response...\n");
            send_response(socket, response_code, filepath, logs);
            free(filepath);
            print(logs, "Response sent successfully!\n");
        }
    }
    
    /* Closes everything: socket, logs and thread. */
    close(socket);
    fclose(logs);
    pthread_exit(0);
}

/**
 * @brief Function that responds to a client.
 * 
 * This function sends headers (depending on response code) and file,
 * which is gotten from filepath.
 * 
 * @param socket Socket to send file to.
 * @param response_code Response code that is used for choosing headers.
 * @param filepath Path to a file to be sent.
 * @return Returns void.
**/
void send_response(int socket, int response_code, char *filepath, FILE *logs){
    /* Creates variables to be used in the future. We can use static memory allocation because */
    /* we set contents of this arrays by ourselves. */
    char current_line[MAX_HEADER_LINE_SIZE] = "";
    char headers[MAX_HEADERS_SIZE] = "";
    char content_type[MAX_CONTENT_TYPE_SIZE] = "";

    /* Gets information about the file. Response code may be changed in the process. */
    unsigned long file_size = get_file_data(filepath, content_type, &response_code);

    /* Saves first line of headers depending on the response code. */
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

        /* This doesn't differ from 404 for now. */
        default:
        {
            strcat(headers, "HTTP/1.1 500 Internal Server Error\n");
            break;
        }
    }

    /* Saves headers to a string. */
    strcat(headers, "Server: Rokoel WebServer\n");
    sprintf(current_line, "Content-length: %lu\n", file_size);
    strcat(headers, current_line);
    sprintf(current_line, "Content-type: %s\n", content_type);
    strcat(headers, current_line);


    print(logs, "SEND_RESPONSE:\n");
    print(logs, "Response headers:\n");
    print(logs, "-----------------\n");
    print(logs, "%s", headers);
    print(logs, "-----------------\n");

    /* Ends headers with empty line. */
    strcat(headers, "\n");
    /* Sends headers alongside with file's contents. */
    send_content(socket, filepath, headers, logs);
}

/**
 * @brief Function that gets information about the file.
 * 
 * This function gets file's content type and its size. It may also change response code.
 * 
 * @param filepath Path to a file to get information about.
 * @param content_type A pointer that will point to file's content type.
 * @param response_code Pointer to response code, because it may be changed.
 * @return Returns file's size.
**/
unsigned long get_file_data(char *filepath, char *content_type, int *response_code){
    /* Gets file data. */
    struct stat file_data;
    /* If somehow file's data cannot be gathered... */
    if (stat(filepath, &file_data) < 0) {
        /* Sets response code to 500 and filepath is now err404's index.html. */
        *response_code = 500;
        strcpy(filepath, ERROR_404_INDEX_HTML_ROUTE);
        /* We can't do much, so it doesn't really matter. */
        return 0;
    }
    
    /* Gets file's content type. */
    if (strstr(filepath, ".html"))
        strcpy(content_type, "text/html");
    else if (strstr(filepath, ".gif"))
        strcpy(content_type, "image/gif");
    else if (strstr(filepath, ".jpg"))
        strcpy(content_type, "image/jpg");
    else if (strstr(filepath, ".png"))
        strcpy(content_type, "image/png");
    else if (strstr(filepath, ".css"))
        strcpy(content_type, "text/css");
    else if (strstr(filepath, ".js"))
        strcpy(content_type, "text/javascript");
    else if (strstr(filepath, ".ico"))
        strcpy(content_type, "image/x-icon");
    else if (strstr(filepath, ".webm"))
        strcpy(content_type, "video/webm");
    else if (strstr(filepath, ".mp4"))
        strcpy(content_type, "video/mp4");
    else if (strstr(filepath, ".mp3"))
        strcpy(content_type, "audio/mp3");
    else if (strstr(filepath, ".wav"))
        strcpy(content_type, "audio/wav");
    else 
        strcpy(content_type, "text/plain");
    
    /* Returns file's size. */
    return (unsigned long)file_data.st_size;
}

/**
 * @brief Function that sends everything to the user.
 * 
 * This function sends headers and file to a socket.
 * 
 * @param socket Socket that everything will be sent to.
 * @param filepath String containing path to a file to be sent.
 * @param headers String containing headers.
 * @return Returns void.
**/
void send_content(int socket, char *filepath, char *headers, FILE *logs){
    short err;
    size_t f_size;
    char * f_data;
    /* Reads a file to a string. */
    f_data = c_read_file(filepath, &err, &f_size, logs);

    /* Writes headers. */
    int n = write(socket, headers, strlen(headers));
    /* If socket would have been blocked, tries again. */
    if(n == -1){
        if(errno == EWOULDBLOCK){
            int i = 0;
            while(n == -1 && errno == EWOULDBLOCK && i < 1000){
                n = write(socket, headers, strlen(headers));
                print(logs, "Trying to write headers again... Wrote %d bytes this time.\n", n);
                ++i;
            }
        }
    }
    print(logs, "Successfully wrote %d bytes of headers.\n", n);
    /* Writes file contents. */
    n = write(socket, f_data, f_size);
    /* If socket would have been blocked, tries again. */
    if(n == -1){
        if(errno == EWOULDBLOCK){
            while(n == -1 && errno == EWOULDBLOCK){
                n = write(socket, f_data, f_size);
                print(logs, "Trying to write file again... Wrote %d bytes this time.\n", n);
            }
        }
    }
    print(logs, "Successfully wrote a file to a socket. It was %d bytes.\n", n);
    /* Frees string containing file's content. */
    free(f_data);
}
