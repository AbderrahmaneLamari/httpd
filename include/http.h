
#ifndef HTTP_H
#define HTTP_H


typedef struct {
    char method[8];
    char url[128];
} httpreq;

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


#endif