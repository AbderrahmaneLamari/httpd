#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../include/http.h"
#include <sys/stat.h>
#include <fcntl.h>




httpreq *parse_http(char *str) {
    httpreq *req = malloc(sizeof(httpreq));
    if (!req) return NULL;
    
    char *p = str;
    
    // Extract method (e.g., "GET")
    while (*p && *p != ' ') p++;
    if (!*p) {
        free(req);
        return NULL;
    }
    *p = 0;
    strncpy(req->method, str, sizeof(req->method) - 1);
    req->method[sizeof(req->method) - 1] = '\0';  // Ensure null termination
    
    // Move to start of URL
    p++;  // Skip the space we just nulled
    str = p;  // ← This is the key fix - update str to point to URL start
    
    // Extract URL (e.g., "/users/9")
    while (*p && *p != ' ') p++;
    if (!*p) {
        free(req);
        return NULL;
    }
    *p = 0;
    strncpy(req->url, str, sizeof(req->url) - 1);
    req->url[sizeof(req->url) - 1] = '\0';  // Ensure null termination
    
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

// ---------------- Utilities ----------------

static char *trim(char *s) {
    while (*s == ' ') s++;
    char *end = s + strlen(s) - 1;
    while (end > s && (*end == ' ' || *end == '\r' || *end == '\n'))
        *end-- = 0;
    return s;
}

static void parse_query(char *path, http_request *req) {
    char *q = strchr(path, '?');
    if (!q) return;

    *q++ = 0;
    req->query_count = 0;

    char *tok = strtok(q, "&");
    while (tok && req->query_count < MAX_QUERY_PARAMS) {
        char *eq = strchr(tok, '=');
        if (eq) {
            *eq = 0;
            snprintf(req->query[req->query_count].key, 32, "%s", tok);
            snprintf(req->query[req->query_count].value, 128, "%s", eq + 1);
            req->query_count++;
        }
        tok = strtok(NULL, "&");
    }
}

// ---------------- Main Parser ----------------

int http_read_request(int c, http_request *req) {
    char buffer[4096];
    int n = read(c, buffer, sizeof(buffer) - 1);

    if (n <= 0)
        return 0;

    buffer[n] = 0;
    memset(req, 0, sizeof(*req));

    char *line = strtok(buffer, "\n");
    if (!line)
        return 0;

    // ---- Request line ----
    sscanf(line, "%7s %255s %15s",
           req->method, req->url, req->version);

    parse_query(req->url, req);

    // ---- Headers ----
    req->header_count = 0;
    while ((line = strtok(NULL, "\n")) && strcmp(line, "\r") != 0) {
        char *colon = strchr(line, ':');
        if (!colon || req->header_count >= MAX_HEADERS)
            continue;

        *colon = 0;
        snprintf(req->headers[req->header_count].key, 32, "%s", trim(line));
        snprintf(req->headers[req->header_count].value, 128,
                 "%s", trim(colon + 1));
        req->header_count++;
    }

    // ---- Body ----
    const char *cl = http_get_header(req, "Content-Length");
    if (cl) {
        int len = atoi(cl);
        if (len > 0 && len < (int)sizeof(req->body)) {
            memcpy(req->body, strtok(NULL, ""), len);
            req->body[len] = 0;
            req->body_length = len;
        }
    }

    return 1;
}

// ---------------- Helpers ----------------

const char *http_get_header(http_request *req, const char *key) {
    for (int i = 0; i < req->header_count; i++) {
        if (strcasecmp(req->headers[i].key, key) == 0)
            return req->headers[i].value;
    }
    return NULL;
}
