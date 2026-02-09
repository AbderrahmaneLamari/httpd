#include "../include/cframework.h"
#include "../include/clog.h"
#include "../include/routes.h"
#include "../include/server.h"
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int start_server(server_t *srv) {

  clog_set_level(CLOG_TRACE);
  clog_set_color_mode(CLOG_COLOR_ALWAYS);

  // Initialize session store
  srv->session_store = session_store_init();
  if (!srv->session_store) {
    ERROR("Failed to initialize session store");
    return -1;
  }
  
  int listen_fd = server_init(srv->port);
  set_nonblocking(listen_fd);

  thread_pool_t pool;
  pool_init(&pool, 8);

  struct pollfd fds[8];
  int nfds;
  fds[0].fd = listen_fd;
  fds[0].events = POLLIN;
  nfds = 1;

  connection_t *conns[8];
  memset(conns, 0, sizeof(conns));

  while (1) {
    int ready = poll(fds, nfds, -1);

    if (ready < 0)
      continue;

    for (int i = 0; i < nfds; i++) {

      if (!(fds[i].revents & POLLIN))
        continue;

      if (fds[i].fd == listen_fd) {
        // Accept new client
        int c = accept(listen_fd, NULL, NULL);
        if (c < 0)
          continue;
        set_nonblocking(c);

        connection_t *conn = calloc(1, sizeof(*conn));
        conn->fd = c;

        if (nfds >= 8) {
          close(c);
          continue;
        }
        fds[nfds].fd = c;
        fds[nfds].events = POLLIN;
        conns[nfds] = conn;
        nfds++;
      } else {
        fds[i].events = 0;
        // Existing client ready
        job_t job = {.conn = conns[i],
                     .routes = srv->routes,
                     .n_routes = srv->route_count,
		     .session_store = srv->session_store };
        pool_submit(&pool, job);
      }
    }

    // Cleanup closed connections
    for (int i = 1; i < nfds; i++) {
      if (conns[i] && conns[i]->closed) {
        close(conns[i]->fd);
        free(conns[i]);

        fds[i] = fds[nfds - 1];
        conns[i] = conns[nfds - 1];
        nfds--;
        i--;
      }
    }
  }
}

void cfk_add_get(server_t *srv, char *path, route_handler handler) {
  if (srv->route_count >= MAX_ROUTE_COUNT) {
    WARN("route_count exceeded MAX_ROUTE_COUNT. route_count=%d",
         srv->route_count);
  }
  struct route rt = {.handler = handler, .method = "GET", .pattern = path};
  DEBUG("Assigning route to server");
  srv->routes[srv->route_count] = rt;
  DEBUG("SERVER ROUTE: %s HAS METHOD: '%s'",
        srv->routes[srv->route_count].pattern,
        srv->routes[srv->route_count].method);
  srv->route_count = srv->route_count + 1;
}
void cfk_add_head(server_t *srv, char *path, route_handler handler) {
  if (srv->route_count >= MAX_ROUTE_COUNT) {
    WARN("route_count exceeded MAX_ROUTE_COUNT. route_count=%d",
         srv->route_count);
  }
  struct route rt = {.handler = handler, .method = "HEAD", .pattern = path};
  DEBUG("SERVER ROUTE: %s HAS METHOD: '%s'",
        srv->routes[srv->route_count].pattern,
        srv->routes[srv->route_count].method);
  srv->routes[srv->route_count++] = rt;
}
void cfk_add_post(server_t *srv, char *path, route_handler handler) {
  if (srv->route_count >= MAX_ROUTE_COUNT) {
    WARN("route_count exceeded MAX_ROUTE_COUNT. route_count=%d",
         srv->route_count);
  }
  struct route rt = {.handler = handler, .method = "POST", .pattern = path};
  DEBUG("SERVER ROUTE: %s HAS METHOD: '%s'",
        srv->routes[srv->route_count].pattern,
        srv->routes[srv->route_count].method);
  srv->routes[srv->route_count++] = rt;
}

void cfk_add_put(server_t *srv, char *path, route_handler handler) {
  if (srv->route_count >= MAX_ROUTE_COUNT) {
    WARN("route_count exceeded MAX_ROUTE_COUNT. route_count=%d",
         srv->route_count);
  }
  struct route rt = {.handler = handler, .method = "POST", .pattern = path};
  DEBUG("SERVER ROUTE: %s HAS METHOD: '%s'",
        srv->routes[srv->route_count].pattern,
        srv->routes[srv->route_count].method);
  srv->routes[srv->route_count++] = rt;
}
void cfk_add_patch(server_t *srv, char *path, route_handler handler) {
  if (srv->route_count >= MAX_ROUTE_COUNT) {
    WARN("route_count exceeded MAX_ROUTE_COUNT. route_count=%d",
         srv->route_count);
  }
  struct route rt = {.handler = handler, .method = "POST", .pattern = path};
  DEBUG("SERVER ROUTE: %s HAS METHOD: '%s'",
        srv->routes[srv->route_count].pattern,
        srv->routes[srv->route_count].method);
  srv->routes[srv->route_count++] = rt;
}
void cfk_add_delete(server_t *srv, char *path, route_handler handler) {
  if (srv->route_count >= MAX_ROUTE_COUNT) {
    WARN("route_count exceeded MAX_ROUTE_COUNT. route_count=%d",
         srv->route_count);
  }
  struct route rt = {.handler = handler, .method = "DELETE", .pattern = path};
  DEBUG("SERVER ROUTE: %s HAS METHOD: '%s'",
        srv->routes[srv->route_count].pattern,
        srv->routes[srv->route_count].method);
  srv->routes[srv->route_count++] = rt;
}
static void add_route(server_t *srv, const char *method, const char *path,
                      route_handler handler, 
                      middleware_fn *middlewares, int middleware_count) {
  if (srv->route_count >= MAX_ROUTE_COUNT) {
    WARN("route_count exceeded MAX_ROUTE_COUNT");
    return;
  }

  struct route rt = {
    .handler = handler,
    .method = method,
    .pattern = path,
    .middleware_count = middleware_count
  };

  // Copy middlewares
  for (int i = 0; i < middleware_count && i < MAX_MIDDLEWARES; i++) {
    rt.middlewares[i] = middlewares[i];
  }

  srv->routes[srv->route_count] = rt;
  srv->route_count++;

  DEBUG("Added route: %s %s with %d middlewares", method, path, middleware_count);
}

// Convenience functions for different HTTP methods with middleware
void cfk_add_get_protected(server_t *srv, char *path, route_handler handler,
                           middleware_fn *middlewares, int count) {
  middleware_fn mw[MAX_MIDDLEWARES];
  mw[0] = require_login;  // Default: require login
  int total = 1;
  
  for (int i = 0; i < count && total < MAX_MIDDLEWARES; i++) {
    mw[total++] = middlewares[i];
  }
  
  add_route(srv, "GET", path, handler, mw, total);
}

void cfk_add_post_protected(server_t *srv, char *path, route_handler handler,
                            middleware_fn *middlewares, int count) {
  middleware_fn mw[MAX_MIDDLEWARES];
  mw[0] = require_login;
  int total = 1;
  
  for (int i = 0; i < count && total < MAX_MIDDLEWARES; i++) {
    mw[total++] = middlewares[i];
  }
  
  add_route(srv, "POST", path, handler, mw, total);
}

void cfk_add_get_admin(server_t *srv, char *path, route_handler handler) {
  middleware_fn mw[] = { require_admin };
  add_route(srv, "GET", path, handler, mw, 1);
}
