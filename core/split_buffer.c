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
 * @return Returns a pointer to an allocated chunk of memory that contains request structure.
*/
struct Request *split_buffer(const char *raw, FILE *logs) {
    if (raw == NULL) {
        if (logs != NULL) {
            print(logs, "Cannot parse a NULL request.\n");
        }
        return NULL;
    }

    struct Request *req = calloc(1, sizeof *req);
    if (req == NULL) {
        print(logs, "Couldn't allocate memory for Request structure.\n");
        return NULL;
    }

    const char *method_end = strchr(raw, ' ');
    if (method_end == NULL || method_end == raw) {
        print(logs, "Request has no valid method separator.\n");
        free_request(req, logs);
        return NULL;
    }

    size_t method_len = (size_t)(method_end - raw);
    if (method_len > 16) {
        print(logs, "Request method is too long.\n");
        free_request(req, logs);
        return NULL;
    }

    /* Chooses one of the methods. */
    if (method_len == 3 && memcmp(raw, "GET", 3) == 0) {
        req->method = GET;
    } else if (method_len == 4 && memcmp(raw, "POST", 4) == 0) {
        req->method = POST;
    } else {
        req->method = UNSUPPORTED;
    }
    print(logs, "Got request method: %d.\n", req->method);

    const char *url_start = method_end + 1;
    const char *url_end = strchr(url_start, ' ');
    if (url_end == NULL || url_end == url_start) {
        print(logs, "Request has no valid URL separator.\n");
        free_request(req, logs);
        return NULL;
    }

    size_t url_len = (size_t)(url_end - url_start);
    if (url_len > 512) {
        print(logs, "Request URL is too long.\n");
        free_request(req, logs);
        return NULL;
    }

    req->url = malloc(url_len + 1);
    if (req->url == NULL) {
        print(logs, "Couldn't allocate memory for a string URL.\n");
        free_request(req, logs);
        return NULL;
    }
    memcpy(req->url, url_start, url_len);
    req->url[url_len] = '\0';
    print(logs, "Got url: %s.\n", req->url);

    const char *version_start = url_end + 1;
    const char *line_end = strstr(version_start, "\r\n");
    size_t line_ending_length = 2;
    if (line_end == NULL) {
        line_end = strchr(version_start, '\n');
        line_ending_length = 1;
    }
    if (line_end == NULL || line_end == version_start) {
        print(logs, "Request has no valid HTTP version line.\n");
        free_request(req, logs);
        return NULL;
    }

    size_t version_len = (size_t)(line_end - version_start);
    if (version_len > 128) {
        print(logs, "HTTP version is too long.\n");
        free_request(req, logs);
        return NULL;
    }

    /* Initializes HTTP-version. */
    req->version = malloc(version_len + 1);
    if (req->version == NULL) {
        print(logs, "Couldn't allocate memory for a string version.\n");
        free_request(req, logs);
        return NULL;
    }
    memcpy(req->version, version_start, version_len);
    req->version[version_len] = '\0';
    print(logs, "Got HTTP-version: %s.\n", req->version);

    const char *headers_start = line_end + line_ending_length;
    const char *headers_end = strstr(headers_start, "\r\n\r\n");
    size_t header_separator_length = 4;
    if (headers_end == NULL) {
        headers_end = strstr(headers_start, "\n\n");
        header_separator_length = 2;
    }

    if (headers_end == NULL) {
        /* A request with no headers may end immediately after the request line. */
        if (*headers_start != '\0') {
            print(logs, "Request has no complete header section.\n");
            free_request(req, logs);
            return NULL;
        }
        headers_end = headers_start;
        header_separator_length = 0;
    }

    size_t headers_size = (size_t)(headers_end - headers_start);
    if (headers_size >= sizeof req->headers) {
        print(logs, "Request headers are too large.\n");
        free_request(req, logs);
        return NULL;
    }
    memcpy(req->headers, headers_start, headers_size);
    req->headers[headers_size] = '\0';
    print(logs, "Got headers: %s\n", req->headers);

    const char *body_start = headers_end + header_separator_length;
    size_t body_len = strlen(body_start);
    req->body = malloc(body_len + 1);
    if (req->body == NULL) {
        print(logs, "Couldn't allocate memory for a string request's body.\n");
        free_request(req, logs);
        return NULL;
    }
    memcpy(req->body, body_start, body_len);
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
    if (req != NULL) {
        if (logs != NULL) {
        print(logs, "Freeing request structure...\n");
        }
        free(req->url);
        free(req->version);
        free(req->body);
        free(req);
    }
}
