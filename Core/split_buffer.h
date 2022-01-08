typedef enum Method {UNSUPPORTED, GET, POST} Method;

/** @brief A structure that contains all information about the request. */
typedef struct Request {
    enum Method method;
    char *url;
    char *version;
    char *body;
    char headers[MAX_HEADERS_SIZE];
} Request;

struct Request *split_buffer(const char *raw, FILE *logs);

void free_request(struct Request *req, FILE *logs);