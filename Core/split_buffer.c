/** @file split_buffer.c
 *  @brief File that contains functions related to splitting incoming requests.
 * 
 *  @author Valentine Y. Shevelev (Rokoel)
 */

#include <stdlib.h>
#include <string.h>

#include "split_buffer.h"


/**
 * @brief Function that saves raw request in the structure.
 * 
 * This function saves buffer containing raw request to a Request structure.
 * 
 * @param raw String that contains raw request.
 * @todo Add support for other ways to end lines ('\r\n' instead of just '\n').
 * @return Returns a pointer to an allocated chunk of memory that contains request structure.
*/
struct Request *split_buffer(const char *raw) {
    /* Creates Request structure and initializes it. */
    struct Request *req = NULL;
    req = malloc(sizeof( *req ));
    if (!req) {
        return NULL;
    }
    memset(req, 0, sizeof( *req ));

    /* Gets method. */
    /* Gets size of the method. */
    size_t method_len = strcspn(raw, " ");
    /* Chooses one of the methods. */
    if (strncmp(raw, "GET", strlen("GET")) == 0) {
        req->method = GET;
    } else if (memcmp(raw, "POST", strlen("POST")) == 0) {
        req->method = POST;
    } else {
        req->method = UNSUPPORTED;
    }
    /* Skips to the part with the url. */
    raw += method_len + 1;

    /* Gets url. */
    /* Gets size of the url. */
    size_t url_len = strcspn(raw, " ");
    /* Inititalizes url. */
    req->url = malloc(url_len + 1);
    if (!req->url) {
        free_request(req);
        return NULL;
    }
    /* Copies url from request. */
    memcpy(req->url, raw, url_len);
    req->url[url_len] = '\0';
    /* Skips to the part with the HTTP-version. */
    raw += url_len + 1;

    /* Gets HTTP-version. */
    /* Gets size of HTTP-version. */
    size_t ver_len = strcspn(raw, "\n");
    /* Initializes HTTP-version. */
    req->version = malloc(ver_len + 1);
    if (!req->version) {
        free_request(req);
        return NULL;
    }
    /* Copies HTTP-version from request. */
    memcpy(req->version, raw, ver_len);
    req->version[ver_len] = '\0';

    /* Skips to the part with headers. */
    raw += ver_len + 1;

    /* Initializes header structure. */
    struct Header *header = NULL, *last = NULL;
    /* Creates singly linked list of headers. */
    while (raw[0]!='\r' || raw[1]!='\n') {
        /* Saves header from the previous loop into the last. */
        last = header;
        /* Initializes current header. */
        header = malloc(sizeof( *header ));
        if (!header) {
            free_request(req);
            return NULL;
        }

        /* Gets size of header's name. */
        size_t name_len = strcspn(raw, ":");
        /* Initializes and saves header's name. */
        header->name = malloc(name_len + 1);
        if (!header->name) {
            free_request(req);
            return NULL;
        }
        memcpy(header->name, raw, name_len);
        /* Closes the name variable. */
        header->name[name_len] = '\0';
        /* Skips to the header's value. */
        raw += name_len + 1; 
        /* Skips all unnecessary spaces. */
        while (*raw == ' ') {
            raw++;
        }

        /* Gets size of header's value. */
        size_t value_len = strcspn(raw, "\n");
        /* Initializes and saves header's value. */
        header->value = malloc(value_len + 1);
        if (!header->value) {
            free_request(req);
            return NULL;
        }
        memcpy(header->value, raw, value_len);
        /* Closes the value variable. */
        header->value[value_len] = '\0';
        /* Skips to the next header. */
        raw += value_len + 1;

        /* Connects headers. */
        header->next = last;
    }

    /* Saves pointer to the first element of singly linked list of headers. */
    req->headers = header;
    /* Skips to the part with body. */
    raw += 2;

    /* Gets the size of request's body. */
    size_t body_len = strlen(raw);
    /* Initializes and saves body. */
    req->body = malloc(body_len + 1);
    if (!req->body) {
        free_request(req);
        return NULL;
    }
    memcpy(req->body, raw, body_len);
    /* Closes body variable. */
    req->body[body_len] = '\0';

    /* Returns a pointer to an allocated chunk of memory that contains request structure. */
    return req;
}

/**
 * @brief Function that frees header structure.
 * 
 * @param h Header structure to be freed.
 * @return Returns void.
*/
void free_header(struct Header *h) {
    if (h) {
        free(h->name);
        free(h->value);
        free_header(h->next);
        free(h);
    }
}

/**
 * @brief Function that frees request structure.
 * 
 * @param req Request structure to be freed.
 * @return Returns void.
*/
void free_request(struct Request *req) {
    if(req){
        free(req->url);
        free(req->version);
        free_header(req->headers);
        free(req->body);
        free(req);
    }
}