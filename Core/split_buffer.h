typedef enum Method {UNSUPPORTED, GET, POST} Method;

/** @brief A structure that contains all information about the header of the request.
 * 
 *  It is supposed to become singly linked list.
*/
typedef struct Header {
    char *name;
    char *value;
    struct Header *next;
} Header;

/** @brief A structure that contains all information about the request. */
typedef struct Request {
    enum Method method;
    char *url;
    char *version;
    struct Header *headers;
    char *body;
} Request;

struct Request *split_buffer(const char *raw);

void free_header(struct Header *h);

void free_request(struct Request *req);