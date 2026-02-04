#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../include/http.h"
#include <sys/stat.h>
#include <fcntl.h>




httpreq *parse_http(char *str) {
    httpreq *req = malloc(sizeof(httpreq));
    char *p = str;

    while (*p && *p != ' ') p++;
    if (!*p) return NULL;
    *p = 0;

    strncpy(req->method, str, sizeof(req->method) - 1);

    str = ++p;
    while (*p && *p != ' ') p++;
    if (!*p) return NULL;
    *p = 0;

    strncpy(req->url, str, sizeof(req->url) - 1);
    return req;
}

char *cli_read(int fd) {
    static char buffer[1024];
    memset(buffer, 0, sizeof(buffer));

    if (read(fd, buffer, sizeof(buffer) - 1) <= 0)
        return NULL;

    return buffer;
}

void http_response(int c, int code, const char *type, const char *body) {
    char buf[2048];
    int len = strlen(body);

    snprintf(buf, sizeof(buf),
        "HTTP/1.1 %d OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s",
        code, type, len, body);

    write(c, buf, strlen(buf));
}

const char *mime_type(const char *path) {
    const char *ext = strrchr(path, '.');
    if (!ext) return "application/octet-stream";

    if (!strcmp(ext, ".html")) return "text/html";
    if (!strcmp(ext, ".css"))  return "text/css";
    if (!strcmp(ext, ".js"))   return "application/javascript";
    if (!strcmp(ext, ".png"))  return "image/png";
    if (!strcmp(ext, ".jpg"))  return "image/jpeg";
    if (!strcmp(ext, ".gif"))  return "image/gif";
    if (!strcmp(ext, ".svg"))  return "image/svg+xml";

    return "application/octet-stream";
}

void http_send_file(int c, const char *path) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        http_response(c, 404, "text/plain", "404 Not Found");
        return;
    }

    struct stat st;
    fstat(fd, &st);

    char header[512];
    snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %ld\r\n"
        "Connection: close\r\n"
        "\r\n",
        mime_type(path),
        st.st_size);

    write(c, header, strlen(header));

    char buf[4096];
    ssize_t n;
    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        write(c, buf, n);
    }

    close(fd);
}
