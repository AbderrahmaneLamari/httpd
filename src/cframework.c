#include "../include/cframework.h"
#include "../include/clog.h"
#include "../include/routes.h"
#include "../include/server.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// starts the server, 0 on error and 1 on success
int start_server(int port) {

  clog_set_level(CLOG_WARN);

  int s = server_init(port);
  printf("Listening on port %d\n", port);

  while (1) {
    int c = server_accept(s);
    if (!fork()) {
      // close(s);
      handle_client(s, c);
      close(c);
      exit(0);
    }
    close(c);
  }
}

// returns 0 on error, else returns 1 on success
int set_routes(route_handler *_route_handelers) {

  route_handlers = _route_handelers;
  return 1;
}