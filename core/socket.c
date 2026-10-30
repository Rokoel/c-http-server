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
 *  @author Rokoel
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

#include <errno.h>    /* errno, EWOULDBLOCK */


#define BUFSIZE 1024
#define MAX_CONTENT_TYPE_SIZE 64
#define MAX_HEADERS_SIZE 700

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
    if (server_descriptor == NULL || *server_descriptor < 0) {
        return;
    }

    /* Creates Socket structure. */
    Socket sock;

    /* Accepts an incoming connection. */
    socklen_t client_length = sizeof sock.address;
    sock.socket_file_descriptor = accept(
        *server_descriptor,
        (struct sockaddr *)&sock.address,
        &client_length
    );

    /* If we got an error when accepting new request... */
    if (sock.socket_file_descriptor == -1) {
        /* And if it is due to the fact that we haven't got any incoming requests... */
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            /* Avoid a tight loop while waiting for another connection. */
            usleep(500000);
        }
        /* Signals can interrupt accept(); this is not a fatal error. */
        else if (errno == EINTR) {
            return;
        }
        /* If this is a different error... */
        else {
            FILE *logs = logs_str != NULL ? fopen(logs_str, "a+") : NULL;
            if (logs != NULL) {
                print(logs, "An error occurred when accepting connection!\n");
                fclose(logs);
            }
        }
    }
    /* If there are no errors... */
    else {
        /* Saves socket file descriptor in newly allocated memory */
        /* for it to exist after compiler reaches end of */
        /* accept_socket_and_open_thread() function. */
        int *pconnfd = malloc(sizeof *pconnfd);
        if (pconnfd == NULL) {
            close(sock.socket_file_descriptor);
            return;
        }

        *pconnfd = sock.socket_file_descriptor;
        /* Opens a new thread. */
        pthread_t t;
        int thread_error = pthread_create(&t, NULL, process_socket, pconnfd);
        if (thread_error != 0) {
            free(pconnfd);
            close(sock.socket_file_descriptor);
            return;
        }

        /* This server does not retain thread IDs to join later. */
        pthread_detach(t);
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
    if (sock == NULL) {
        return NULL;
    }

    int socket = *((int *)sock);
    free(sock);

    char current_log_file[DATE_TIME_LEN + 32];
    char date_and_time[DATE_TIME_LEN];
    /* Gets today's date that will be used in naming current log file. */
    if (!format_current_time_date(date_and_time, 0)) {
        close(socket);
        return NULL;
    }
    /* Sets current_log_file to today's log filename. */
    if (snprintf(current_log_file, sizeof current_log_file, LOGS_LOCATION,
                 date_and_time) >= (int)sizeof current_log_file) {
        close(socket);
        return NULL;
    }
    /* Opens logs file. */
    FILE *logs = fopen(current_log_file, "a+");
    if (logs == NULL) {
        close(socket);
        return NULL;
    }
    /* Creating and filling buffer with zeros. */
    char buffer[BUFSIZE];
    memset(buffer, 0, sizeof buffer);
    
    /* Accumulate a request because one read() may contain only part of it. */
    size_t bytes_received = 0;
    ssize_t bytes_read = 0;
    int error_max_counter = 500;
    int error_counter = 0;
    int read_failed = 0;
    while (bytes_received < sizeof buffer - 1) {
        bytes_read = read(socket, buffer + bytes_received,
                          sizeof buffer - 1 - bytes_received);
        if (bytes_read > 0) {
            bytes_received += (size_t)bytes_read;
            buffer[bytes_received] = '\0';
            if (strstr(buffer, "\r\n\r\n") != NULL ||
                strstr(buffer, "\n\n") != NULL) {
                break;
            }
            continue;
        }

        if (bytes_read == 0 || errno == EINTR) {
            if (bytes_read == 0) {
                break;
            }
            continue;
        }

        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            if (error_counter++ >= error_max_counter) {
                read_failed = 1;
                break;
            }
            usleep(10000);
            continue;
        }

        read_failed = 1;
        break;
    }

    bytes_read = read_failed ? -1 : (ssize_t)bytes_received;
    /* If an error still remains or if it occured just now, we print about it and close logs, socket and everything. */
    if (bytes_read == -1) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            print(logs, "An unexpected error occurred while reading from the socket.\n");
        }
        else {
            print(logs, "Has been waiting for a response for too long.\n");
        }
    }
    else if (bytes_read == 0) {
        print(logs, "The client closed the connection without sending data.\n");
    }
    /* If everything is fine... */
    else {
        /* Tells how many bytes it read. */
        print(logs, "Successfully read %zd bytes.\n", bytes_read);
        buffer[bytes_read] = '\0';
        
        print(logs, "Received from client: \n%s\n", buffer);

        /* Tries to parse the request into a structure. */
        print(logs, "What was processed into a structure:\n");
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
            int response_code = get_filepath(req->url, filepath, sizeof filepath, logs);
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
            char filepath[1024];

            /* Gets filepath from the buffer and response code into variables. */
            int response_code = get_filepath("/err404/index.html", filepath, sizeof filepath, logs);
            print(logs, "Response code is %d.\n", response_code);
            
            /* Sends response to the client (headers and requested content). */
            print(logs, "Sending response...\n");
            send_response(socket, response_code, filepath, logs);
            print(logs, "Response sent successfully!\n");
        }
    }
    
    /* Closes everything: socket, logs and thread. */
    close(socket);
    fclose(logs);
    return NULL;
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
    char headers[MAX_HEADERS_SIZE] = "";
    char content_type[MAX_CONTENT_TYPE_SIZE] = "";

    /* Gets information about the file. Response code may be changed in the process. */
    unsigned long file_size = get_file_data(filepath, content_type, &response_code);

    const char *status_line;
    switch (response_code) {
        case 200:
            status_line = "HTTP/1.1 200 OK\r\n";
            break;
        case 404:
            status_line = "HTTP/1.1 404 Not Found\r\n";
            break;
        default:
            status_line = "HTTP/1.1 500 Internal Server Error\r\n";
            break;
    }

    int header_length = snprintf(
        headers, sizeof headers,
        "%sServer: Rokoel WebServer\r\n"
        "Content-Length: %lu\r\n"
        "Content-Type: %s\r\n\r\n",
        status_line, file_size, content_type
    );
    if (header_length < 0 || (size_t)header_length >= sizeof headers) {
        print(logs, "Response headers are too large.\n");
        return;
    }


    print(logs, "SEND_RESPONSE:\n");
    print(logs, "Response headers:\n");
    print(logs, "-----------------\n");
    print(logs, "%s", headers);
    print(logs, "-----------------\n");

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
    if (filepath == NULL || content_type == NULL || response_code == NULL) {
        return 0;
    }

    /* Gets file data. */
    struct stat file_data;
    /* If somehow file's data cannot be gathered... */
    if (stat(filepath, &file_data) < 0) {
        /* Sets response code to 500 and filepath is now err404's index.html. */
        *response_code = 500;
        strcpy(filepath, ERROR_404_INDEX_HTML_ROUTE);
        if (stat(filepath, &file_data) < 0) {
            content_type[0] = '\0';
            return 0;
        }
    }

    if (!S_ISREG(file_data.st_mode)) {
        *response_code = 500;
        content_type[0] = '\0';
        return 0;
    }
    
    /* Gets file's content type. */
    const char *extension = strrchr(filepath, '.');
    if (extension != NULL && strcmp(extension, ".html") == 0)
        strcpy(content_type, "text/html");
    else if (extension != NULL && strcmp(extension, ".gif") == 0)
        strcpy(content_type, "image/gif");
    else if (extension != NULL &&
             (strcmp(extension, ".jpg") == 0 || strcmp(extension, ".jpeg") == 0))
        strcpy(content_type, "image/jpeg");
    else if (extension != NULL && strcmp(extension, ".png") == 0)
        strcpy(content_type, "image/png");
    else if (extension != NULL && strcmp(extension, ".css") == 0)
        strcpy(content_type, "text/css");
    else if (extension != NULL && strcmp(extension, ".js") == 0)
        strcpy(content_type, "text/javascript");
    else if (extension != NULL && strcmp(extension, ".ico") == 0)
        strcpy(content_type, "image/x-icon");
    else if (extension != NULL && strcmp(extension, ".webm") == 0)
        strcpy(content_type, "video/webm");
    else if (extension != NULL && strcmp(extension, ".mp4") == 0)
        strcpy(content_type, "video/mp4");
    else if (extension != NULL && strcmp(extension, ".mp3") == 0)
        strcpy(content_type, "audio/mp3");
    else if (extension != NULL && strcmp(extension, ".wav") == 0)
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
static int write_all(int socket, const void *data, size_t length){
    const char *bytes = data;
    size_t written = 0;
    unsigned int retries = 0;

    while (written < length) {
        ssize_t result = write(socket, bytes + written, length - written);
        if (result > 0) {
            written += (size_t)result;
            retries = 0;
            continue;
        }

        if (result == -1 && errno == EINTR) {
            continue;
        }

        if (result == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            if (retries++ >= 1000) {
                return -1;
            }
            usleep(1000);
            continue;
        }

        return -1;
    }

    return 0;
}

void send_content(int socket, char *filepath, char *headers, FILE *logs){
    short err;
    size_t f_size;
    char * f_data;
    /* Reads a file to a string. */
    f_data = c_read_file(filepath, &err, &f_size, logs);
    if (f_data == NULL) {
        print(logs, "Could not read '%s'; response was not sent.\n", filepath);
        return;
    }

    /* Writes headers. */
    size_t headers_size = strlen(headers);
    if (write_all(socket, headers, headers_size) == -1) {
        print(logs, "Could not write response headers to the socket.\n");
        free(f_data);
        return;
    }
    print(logs, "Successfully wrote %zu bytes of headers.\n", headers_size);

    /* Writes file contents. */
    if (write_all(socket, f_data, f_size) == -1) {
        print(logs, "Could not write file contents to the socket.\n");
        free(f_data);
        return;
    }
    print(logs, "Successfully wrote %zu bytes of file contents to a socket.\n", f_size);
    /* Frees string containing file's content. */
    free(f_data);
}
