
#ifndef HTTP_H
#define HTTP_H

#include "session.h"
#define MAX_HEADERS 0x20       // 32
#define MAX_QUERY_PARAMS 0x10  // 16
#define MAX_BUFFER_SIZE 0x1000 // 4096
#define MAX_BODY_SIZE 0x400    // 1024
#define HTTP_MAX_HEADERS 0x20  // 32
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

  char body[MAX_BODY_SIZE];
  int body_length;
  int keep_alive;
  session_t *session;                     // Add this
  char session_id[SESSION_ID_LENGTH + 1]; // Add this
} http_request;

typedef enum {

  /* 1xx — Informational */
  HTTP_CONTINUE = 100,
  HTTP_SWITCHING_PROTOCOLS = 101,
  HTTP_PROCESSING = 102,
  HTTP_EARLY_HINTS = 103,

  /* 2xx — Success */
  HTTP_OK = 200,
  HTTP_CREATED = 201,
  HTTP_ACCEPTED = 202,
  HTTP_NON_AUTHORITATIVE_INFORMATION = 203,
  HTTP_NO_CONTENT = 204,
  HTTP_RESET_CONTENT = 205,
  HTTP_PARTIAL_CONTENT = 206,
  HTTP_MULTI_STATUS = 207,
  HTTP_ALREADY_REPORTED = 208,
  HTTP_IM_USED = 226,

  /* 3xx — Redirection */
  HTTP_MULTIPLE_CHOICES = 300,
  HTTP_MOVED_PERMANENTLY = 301,
  HTTP_FOUND = 302,
  HTTP_SEE_OTHER = 303,
  HTTP_NOT_MODIFIED = 304,
  HTTP_USE_PROXY = 305,
  HTTP_TEMPORARY_REDIRECT = 307,
  HTTP_PERMANENT_REDIRECT = 308,

  /* 4xx — Client Error */
  HTTP_BAD_REQUEST = 400,
  HTTP_UNAUTHORIZED = 401,
  HTTP_PAYMENT_REQUIRED = 402,
  HTTP_FORBIDDEN = 403,
  HTTP_NOT_FOUND = 404,
  HTTP_METHOD_NOT_ALLOWED = 405,
  HTTP_NOT_ACCEPTABLE = 406,
  HTTP_PROXY_AUTH_REQUIRED = 407,
  HTTP_REQUEST_TIMEOUT = 408,
  HTTP_CONFLICT = 409,
  HTTP_GONE = 410,
  HTTP_LENGTH_REQUIRED = 411,
  HTTP_PRECONDITION_FAILED = 412,
  HTTP_PAYLOAD_TOO_LARGE = 413,
  HTTP_URI_TOO_LONG = 414,
  HTTP_UNSUPPORTED_MEDIA_TYPE = 415,
  HTTP_RANGE_NOT_SATISFIABLE = 416,
  HTTP_EXPECTATION_FAILED = 417,
  HTTP_IM_A_TEAPOT = 418,

  HTTP_MISDIRECTED_REQUEST = 421,
  HTTP_UNPROCESSABLE_ENTITY = 422,
  HTTP_LOCKED = 423,
  HTTP_FAILED_DEPENDENCY = 424,
  HTTP_TOO_EARLY = 425,
  HTTP_UPGRADE_REQUIRED = 426,
  HTTP_PRECONDITION_REQUIRED = 428,
  HTTP_TOO_MANY_REQUESTS = 429,
  HTTP_REQUEST_HEADER_FIELDS_TOO_LARGE = 431,
  HTTP_UNAVAILABLE_FOR_LEGAL_REASONS = 451,

  /* 5xx — Server Error */
  HTTP_INTERNAL_SERVER_ERROR = 500,
  HTTP_NOT_IMPLEMENTED = 501,
  HTTP_BAD_GATEWAY = 502,
  HTTP_SERVICE_UNAVAILABLE = 503,
  HTTP_GATEWAY_TIMEOUT = 504,
  HTTP_HTTP_VERSION_NOT_SUPPORTED = 505,
  HTTP_VARIANT_ALSO_NEGOTIATES = 506,
  HTTP_INSUFFICIENT_STORAGE = 507,
  HTTP_LOOP_DETECTED = 508,
  HTTP_NOT_EXTENDED = 510,
  HTTP_NETWORK_AUTHENTICATION_REQUIRED = 511

} http_status_t;

typedef struct {
  int client_fd;
  int status;

  http_pair headers[MAX_HEADERS];
  int header_count;

  int headers_sent;
} http_response;

// void http_response(
//     int client_fd,
//     int status,
//     const char *content_type,
//     const char *body
// );

void http_res_init(http_response *res, int client_fd);
void http_res_set_header(http_response *res, const char *key,
                         const char *value);
void http_res_send(http_response *res, int status, const char *content_type,
                   const char *body);

void http_send_file(http_request *req, http_response *res);

const char *mime_type(const char *path);

int http_read_request(int client_fd, http_request *req);
const char *http_get_header(http_request *req, const char *key);

#endif