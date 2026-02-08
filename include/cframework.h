#ifndef CFRAMEWORK
#define CFRAMEWORK

#include "routes.h"
typedef struct server_st {
  int listen_fd;
  int port;
  int backlog;
  struct sockaddr_in *addr;

  struct route routes[MAX_ROUTE_COUNT];
  int route_count;

  const char *static_url;  // e.g. "/static/"
  const char *static_root; // e.g. "./www"

  int running;

  const char *server_name;
} server_t;

// int server_init(server_t *srv, int port);
// int start_server(int port, struct route routes[], int n);
int start_server(server_t *srv);
int set_routes(struct route *_routes);

void cfk_add_get(server_t *srv, char *path, route_handler handler);
void cfk_add_head(server_t *srv, char *path, route_handler handler);
void cfk_add_post(server_t *srv, char *path, route_handler handler);

void cfk_add_put(server_t *srv, char *path, route_handler handler);
void cfk_add_patch(server_t *srv, char *path, route_handler handler);
void cfk_add_delete(server_t *srv, char *path, route_handler handler);

// static struct route *routes;

#endif