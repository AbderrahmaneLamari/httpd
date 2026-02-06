#include "../include/server.h"
#include "../include/http.h"
#include "../include/routes.h"
#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

int server_init(int port) {
  int s = socket(AF_INET, SOCK_STREAM, 0);

  struct sockaddr_in addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(port);

  int one = 1;
  setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

  int bind_res = bind(s, (struct sockaddr *)&addr, sizeof(addr));
  if (bind_res) {
    fprintf(stderr, "bind() error");
    exit(1);
  }
  listen(s, 10);

  return s;
}

int server_accept(int s) {

  // client socket address
  struct sockaddr_in cli;
  socklen_t len = sizeof(cli);
  
  return accept(s, (struct sockaddr *)&cli, &len);
}

void handle_client(int s, int c, struct route routes[], int n) {
  
  http_request req;
  http_read_request(c, &req);
  dispatch_route(c, &req, routes, n);

}
