/** @file split_buffer.c
 *  @brief File that contains functions related to splitting incoming requests.
 * 
 *  @author Valentine Y. Shevelev (Rokoel)
**/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "helpful.h"
#include "split_buffer.h"


/**
 * @brief Function that saves raw request in the structure.
 * 
 * This function saves buffer containing raw request to a Request structure.
 * 
 * @param raw String that contains raw request.
 * @param logs File pointer containing logs.
 * @todo Add support for other ways to end lines ('\r\n' instead of just '\n').
 * @return Returns a pointer to an allocated chunk of memory that contains request structure.
*/
struct Request *split_buffer(const char *raw, FILE *logs) {
    /* Creates Request structure and initializes it. */
    struct Request *req = NULL;
    req = malloc(sizeof( *req ));
    if (!req) {
        print(logs, "Couldn't allocate memory for Request structure.\n");
        return NULL;
    }
    memset(req, 0, sizeof( *req ));

    /* Gets method. */
    /* Gets size of the method. */
    size_t method_len = strcspn(raw, " ");
    if(method_len > 128){
        print(logs, "Normal method can't be THIS big.\n");
        free_request(req, logs);
        return NULL;
    }
    /* Chooses one of the methods. */
    if (strncmp(raw, "GET", strlen("GET")) == 0) {
        req->method = GET;
    } else if (strncmp(raw, "POST", strlen("POST")) == 0) {
        req->method = POST;
    } else {
        req->method = UNSUPPORTED;
    }
    print(logs, "Got request method: %d.\n", req->method);
    /* Skips to the part with the url. */
    raw += method_len + 1;

    /* Gets url. */
    /* Gets size of the url. */
    size_t url_len = strcspn(raw, " ");
    if(url_len > 512){
        print(logs, "Normal url can't be THIS big.\n");
        free_request(req, logs);
        return NULL;
    }
    /* Inititalizes url. */
    req->url = malloc(url_len + 1);
    if (!req->url) {
        print(logs, "Couldn't allocate memory for a string url.\n");
        free_request(req, logs);
        return NULL;
    }
    /* Copies url from request. */
    strncpy(req->url, raw, url_len);
    req->url[url_len] = '\0';
    /* Skips to the part with the HTTP-version. */
    raw += url_len + 1;
    print(logs, "Got url: %s.\n", req->url);

    /* Gets HTTP-version. */
    /* Gets size of HTTP-version. */
    size_t ver_len = strcspn(raw, "\n");
    if(ver_len > 128){
        print(logs, "Normal header can't be THIS big.\n");
        free_request(req, logs);
        return NULL;
    }
    /* Initializes HTTP-version. */
    req->version = malloc(ver_len + 1);
    if (!req->version) {
        print(logs, "Couldn't allocate memory for a string version.\n");
        free_request(req, logs);
        return NULL;
    }
    /* Copies HTTP-version from request. */
    strncpy(req->version, raw, ver_len);
    req->version[ver_len] = '\0';
    print(logs, "Got HTTP-version: %s.\n", req->version);

    /* Skips to the part with headers. */
    raw += ver_len + 1;

    size_t headers_size = strspn(raw, "\r\n");
    if(MAX_HEADERS_SIZE - 1 <= headers_size){
        strncpy(req->headers, raw, MAX_HEADERS_SIZE-1);
        req->headers[MAX_HEADERS_SIZE-1] = '\0';
    }
    else{
        strncpy(req->headers, raw, headers_size);
        req->headers[headers_size] = '\0';
    }
    print(logs, "Got headers: %s\n", req->headers);
    /* Skips to the part with body. */
    raw += strlen(req->headers) + 2;

    /* Gets the size of request's body. */
    size_t body_len = strlen(raw);
    /* Initializes and saves body. */
    req->body = malloc(body_len + 1);
    if (!req->body) {
        print(logs, "Couldn't allocate memory for a string request's body.\n");
        free_request(req, logs);
        return NULL;
    }
    strncpy(req->body, raw, body_len);
    /* Closes body variable. */
    req->body[body_len] = '\0';
    print(logs, "Got request's body.\n");

    /* Returns a pointer to an allocated chunk of memory that contains request structure. */
    return req;
}

/**
 * @brief Function that frees request structure.
 * 
 * @param req Request structure to be freed.
 * @param logs File pointer containing logs.
 * @return Returns void.
*/
void free_request(struct Request *req, FILE *logs) {
    if(req){
        print(logs, "Freeing request structure...\n");
        free(req->url);
        free(req->version);
        free(req->body);
        free(req);
    }
}