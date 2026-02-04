
#ifndef HTTP_H
#define HTTP_H

#define MAX_HEADERS 32
#define MAX_QUERY_PARAMS  16

typedef struct {
    char method[8];
    char url[128];
} httpreq;

typedef struct {
    char key[32];
    char value[128];
} http_pair;

typedef struct {
    char method[8];
    char url[256];
    char version[16];

    http_pair headers[MAX_HEADERS];
    int header_count;

    http_pair query[MAX_QUERY_PARAMS];
    int query_count;

    char body[1024];
    int body_length;
} http_request;

httpreq *parse_http(char *raw);
char *cli_read(int client_fd);

void http_response(
    int client_fd,
    int status,
    const char *content_type,
    const char *body
);

void http_send_file(int client_fd, const char *path);

const char *mime_type(const char *path);

int http_read_request(int client_fd, http_request *req);
const char *http_get_header(http_request *req, const char *key);

#endif