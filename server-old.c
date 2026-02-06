#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define LISTENADDRES "0.0.0.0"

/* Structures */
struct sHttpRequest {
  char method[8];
  char url[128];
};

/* Type declarations*/
typedef struct sHttpRequest httpreq;

/* function declarations*/
httpreq *parse_http(char *str);
void cli_conn(int s, int c);
int cli_accept(int s);
int srv_init(int portno);
char *cli_read(int c);

/* Global */
char *error;

/* returns 0 on error or it retuns an "httpreq" struct*/
httpreq *parse_http(char *str) {
  httpreq *req;
  char *p;

  req = malloc(sizeof(httpreq));

  for (p = str; *p && *p != ' '; p++)
    ;
  if (*p == ' ')
    *p = 0;
  else {
    printf("DEBUG");
    fflush(stdout);
    error = "http_parse() NOSPACE error";
    free(req);
    return 0;
  }
  strncpy(req->method, str, 7);
  // Getting the url part of the request
  for (str = ++p; *p && *p != ' '; p++)
    ;
  if (*p == ' ')
    *p = 0;
  else {
    printf("DEBUG");
    fflush(stdout);
    error = "http_parse() NOSPACE error";
    free(req);
    return 0;
  }

  strncpy(req->url, str, 127);
  return req;
}
/* returns 0 on error or returns the data on success*/
char *cli_read(int c) {
  static char buffer[512];

  memset(buffer, 0, 512);
  int read_res = read(c, buffer, 511);

  if (read_res < 0) {
    error = "cli_read() error";
    return 0;
  } else {
    return buffer;
  }
}

void http_headers(int c, int code) {
  char buf[512];
  int n;

  memset(buf, 0, 512);

  snprintf(buf, 511,
           "HTTP/1.1 %d \r\n"
           "location: rigth-over-here\r\n"
           "content-type: text/html; charset=UTF-8\r\n"
           "expires: -1\r\n"
           "server: httpd.local\r\n",
           code);

  n = strlen(buf);
  write(c, buf, n);

  return;
}
void http_response(int c, char *contenttype, char *data) {
  char buf[512];
  int n;

  n = strlen(data);
  memset(buf, 0, 512);
  snprintf(buf, 511,
           "content-type: %s\n"
           "content-length: %d\n"
           "\n%s\n",
           contenttype, n, data);
  n = strlen(buf);
  write(c, buf, n);
  return;
}

void cli_conn(int s, int c) {

  httpreq *req;
  char buf[512];
  char *p;
  char *res;

  p = cli_read(c);

  if (!p) {
    fprintf(stderr, "%s\n", error);
    close(c);
    return;
  }

  req = parse_http(p);
  if (!req) {
    fprintf(stderr, "%s\n", error);
    close(c);
    return;
  }

  if (!strcmp(req->method, "GET") && (!strcmp(req->url, "/app/webpage"))) {
    res = "<html><head></head><body><h1>Hello world from httpd!</h1></body></html>";
    http_headers(c, 200);
    http_response(c, "text/html", res);
  } else {
    res = "File not found";
    http_headers(c, 404);
    http_response(c, "text/plain", res);
  }

  free(req);
  close(c);
}

// return 0 on error, else returns a file descriptor
int cli_accept(int s) {

  int c;
  struct sockaddr_in cli;
  socklen_t addrlen = sizeof(cli);

  memset(&cli, 0, sizeof(cli));
  c = accept(s, (struct sockaddr *)&cli, &addrlen);

  if (c < 0) {
    error = "accept() error";
    return 0;
  }
  return c;
}

// return 0 on error, else returns a file descriptor
int srv_init(int portno) {
  int s;
  struct sockaddr_in srv;

  s = socket(AF_INET, SOCK_STREAM, 0);

  if (s < 0) {
    error = "socket() error";
    return 0;
  }

  srv.sin_family = AF_INET;
  srv.sin_addr.s_addr = inet_addr(LISTENADDRES);
  srv.sin_port = htons(portno);

  if (bind(s, (struct sockaddr *)&srv, sizeof(srv))) {
    {
      error = "bind() error";
      close(s);
      return 0;
    }
  }

  if (listen(s, 5)) {
    close(s);
    error = "listen() error";
    return 0;
  }

  return s;
}

int main(int argc, char **argv) {
  int s, c;
  char *port;

  if (argc < 2) {
    fprintf(stderr, "Usage: %s <listening>\n", argv[0]);
    return -1;
  } else {
    port = argv[1];
  }
  s = srv_init(atoi(port));

  if (!s) {
    fprintf(stderr, "%s\n", error);
    return -1;
  }

  printf("Listening on %s:%s\n", LISTENADDRES, port);

  while (1) {
    c = cli_accept(s);

    if (c < 0) {
      fprintf(stderr, "%s\n", error);
      continue;
    }

    printf("Incoming connection...\n");
    if (!fork()) {
      cli_conn(s, c);
    }
  }

  // end of the program.
  // should not reach this point.
  return -1;
}

/*

HTTP/1.1 301
location: https://www.google.com/
content-type: text/html; charset=UTF-8
expires: Fri, 06 Mar 2026 10:39:54 GMT
cache-control: public, max-age=2592000
server: gws
content-length: 220

*/