#include "../include/http.h"
#include "../include/clog.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

httpreq *parse_http(char *str) {
  httpreq *req = malloc(sizeof(httpreq));
  if (!req)
    return NULL;

  char *p = str;

  // Extract method (e.g., "GET")
  while (*p && *p != ' ')
    p++;
  if (!*p) {
    free(req);
    return NULL;
  }
  *p = 0;
  strncpy(req->method, str, sizeof(req->method) - 1);
  req->method[sizeof(req->method) - 1] = '\0'; // Ensure null termination

  // Move to start of URL
  p++;     // Skip the space we just nulled
  str = p; // ← This is the key fix - update str to point to URL start

  // Extract URL (e.g., "/users/9")
  while (*p && *p != ' ')
    p++;
  if (!*p) {
    free(req);
    return NULL;
  }
  *p = 0;
  strncpy(req->url, str, sizeof(req->url) - 1);
  req->url[sizeof(req->url) - 1] = '\0'; // Ensure null termination

  return req;
}

char *cli_read(int fd) {
  static char buffer[1024];
  memset(buffer, 0, sizeof(buffer));

  if (read(fd, buffer, sizeof(buffer) - 1) <= 0)
    return NULL;

  return buffer;
}

// void http_response(http_request *req, http_response *res) {
//     char buf[2048];
//     int len = strlen(body);
//     snprintf(buf, sizeof(buf),
//         "HTTP/1.1 %d OK\r\n"
//         "Content-Type: %s\r\n"
//         "Content-Length: %d\r\n"
//         "Connection: close\r\n"
//         "\r\n"
//         "%s",
//         code, type, len, body);
//     write(c, buf, strlen(buf));
// }

void http_res_init(http_response *res, int client_fd) {
  memset(res, 0, sizeof(*res));
  res->client_fd = client_fd;
}

void http_res_set_header(http_response *res, const char *key,
                         const char *value) {
  if (res->header_count >= HTTP_MAX_HEADERS)
    return; // or assert / error

  strncpy(res->headers[res->header_count].key, key,
          sizeof(res->headers[res->header_count].key) - 1);
  res->headers[res->header_count]
      .key[sizeof(res->headers[res->header_count].key) - 1] = '\0';

  strncpy(res->headers[res->header_count].value, value,
          sizeof(res->headers[res->header_count].value) - 1);
  res->headers[res->header_count]
      .value[sizeof(res->headers[res->header_count].value) - 1] = '\0';

  res->header_count++;
}

static const char *http_reason_phrase(int status) {
  switch (status) {
  case 200:
    return "OK";
  case 201:
    return "Created";
  case 204:
    return "No Content";
  case 301:
    return "Moved Permanently";
  case 302:
    return "Found";
  case 400:
    return "Bad Request";
  case 403:
    return "Forbidden";
  case 404:
    return "Not Found";
  case 405:
    return "Method Not Allowed";
  case 413:
    return "Payload Too Large";
  case 500:
    return "Internal Server Error";
  case 501:
    return "Not Implemented";
  default:
    return "Unknown";
  }
}

void http_res_send(http_response *res, int status, const char *content_type,
                   const char *body) {
  if (res->headers_sent)
    return;

  res->status = status;
  const char *reason = http_reason_phrase(status);
  int body_len = body ? strlen(body) : 0;

  char buffer[1024];
  int len = 0;

  // Status line
  len += snprintf(buffer + len, sizeof(buffer) - len, "HTTP/1.1 %d %s\r\n",
                  status, reason);

  // Mandatory headers
  len += snprintf(buffer + len, sizeof(buffer) - len,
                  "Content-Length: %d\r\n"
                  "Content-Type: %s\r\n",
                  body_len, content_type ? content_type : "text/plain");

  // Custom headers
  for (int i = 0; i < res->header_count; i++) {
    len += snprintf(buffer + len, sizeof(buffer) - len, "%s: %s\r\n",
                    res->headers[i].key, res->headers[i].value);
  }

  // End headers
  len += snprintf(buffer + len, sizeof(buffer) - len, "\r\n");

  // Send headers
  write(res->client_fd, buffer, len);

  // Send body
  if (body_len > 0)
    write(res->client_fd, body, body_len);

  res->headers_sent = 1;
}

const char *mime_type(const char *path) {
  const char *ext = strrchr(path, '.');
  if (!ext)
    return "application/octet-stream";

  if (!strcmp(ext, ".html"))
    return "text/html";
  if (!strcmp(ext, ".css"))
    return "text/css";
  if (!strcmp(ext, ".js"))
    return "application/javascript";
  if (!strcmp(ext, ".png"))
    return "image/png";
  if (!strcmp(ext, ".jpg"))
    return "image/jpeg";
  if (!strcmp(ext, ".gif"))
    return "image/gif";
  if (!strcmp(ext, ".svg"))
    return "image/svg+xml";

  return "application/octet-stream";
}

void http_send_file(http_request *req, http_response *res) {
  int fd = open(req->url, O_RDONLY);
  if (fd < 0) {
    http_res_send(res, 404, "text/plain", NULL);
    return;
  }

  struct stat st;
  fstat(fd, &st);

  char header[512];
  snprintf(header, sizeof(header),
           "HTTP/1.1 200 OK\r\n"
           "Content-Type: %s\r\n"
           "Content-Length: %ld\r\n"
           "Connection: keep-alive\r\n"
           "\r\n",
           mime_type(req->url), st.st_size);

  write(res->client_fd, header, strlen(header));

  char buf[4096];
  ssize_t n;
  while ((n = read(fd, buf, sizeof(buf))) > 0) {
    write(res->client_fd, buf, n);
  }

  close(fd);
}

// ---------------- Utilities ----------------

static char *trim(char *s) {
  while (*s == ' ')
    s++;
  char *end = s + strlen(s) - 1;
  while (end > s && (*end == ' ' || *end == '\r' || *end == '\n'))
    *end-- = 0;
  return s;
}

static void parse_query(char *path, http_request *req) {
  char *q = strchr(path, '?');
  if (!q)
    return;

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

// returns 0 on error, else number of byte read from stream
int http_read_request(int c, http_request *req) {
  char buffer[MAX_HEADERS];
  int n = read(c, buffer, sizeof(buffer) - 1);

  DEBUG("Read %d bytes from socket", n);

  if (n == 0) {
    return 0;
  }

  if (n < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return -2;
    }
    return -1;
  }

  buffer[n] = 0;
  
  DEBUG("Raw buffer: [%s]", buffer);
  
  memset(req, 0, sizeof(*req));

  char *line = strtok(buffer, "\n");
  if (!line) {
    DEBUG("Failed to get first line");
    return -1;
  }

  DEBUG("First line: [%s]", line);

  // ---- Request line ----
  // IMPORTANT: Use the req struct's buffers, not pointers into buffer!
  int parsed = sscanf(line, "%7s %255s %15s", req->method, req->url, req->version);
  
  DEBUG("Parsed %d fields: method='%s', url='%s', version='%s'", 
        parsed, req->method, req->url, req->version);
  
  if (parsed < 2) {
    DEBUG("Parse failed, only got %d fields", parsed);
    return -1;
  }

  parse_query(req->url, req);

  // ---- Headers ----
  req->header_count = 0;
  while ((line = strtok(NULL, "\n")) && strcmp(line, "\r") != 0) {
    char *colon = strchr(line, ':');
    if (!colon || req->header_count >= MAX_HEADERS)
      continue;

    *colon = 0;
    // Copy into req's header buffers, not use pointers
    snprintf(req->headers[req->header_count].key, 32, "%s", trim(line));
    snprintf(req->headers[req->header_count].value, 128, "%s", trim(colon + 1));
    req->header_count++;
  }

  // ---- Body ----
  const char *cl = http_get_header(req, "Content-Length");
  if (cl) {
    int len = atoi(cl);
    if (len > 0 && len < (int)sizeof(req->body)) {
      char *body_start = strtok(NULL, "");
      if (body_start) {
        memcpy(req->body, body_start, len);
        req->body[len] = 0;
        req->body_length = len;
      }
    }
  }
  
  const char *ka = http_get_header(req, "Connection");
  if (ka && strcmp(ka, "keep-alive") == 0) {
    req->keep_alive = 1;
  }

  DEBUG("Final URL after query parse: '%s'", req->url);

  return n;
}

// ---------------- Helpers ----------------

const char *http_get_header(http_request *req, const char *key) {

  for (int i = 0; i < req->header_count; i++) {

    if (strcasecmp(req->headers[i].key, key) == 0)
      return req->headers[i].value;
  }
  return NULL;
}
