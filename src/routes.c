#include "../include/routes.h"
#include "../include/http.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/clog.h"

#define STATIC_ROOT "./www"

// ------------------ Handlers ------------------



static void not_found(int c, route_params *p, http_request* req) {
  http_response(c, 404, "text/plain", "404 Not Found");
}

// ------------------ Static File ------------------

static void static_handler(int c, const char *url) {
  char path[512];

  // Prevent directory traversal
  if (strstr(url, "..")) {
    http_response(c, 403, "text/plain", "403 Forbidden");
    return;
  }

  // remove /static prefix
  snprintf(path, sizeof(path), "%s%s", STATIC_ROOT, url + 7);
  http_send_file(c, path);
}

// ------------------ Route Matching ------------------

static int match_route(const char *pattern, const char *path, route_params *out) {
    char pcopy[256], ucopy[256];
    char *ptok, *utok;
    char *psave, *usave;
    out->count = 0;

    snprintf(pcopy, sizeof(pcopy), "%s", pattern);
    snprintf(ucopy, sizeof(ucopy), "%s", path);

    DEBUG("Matching pattern='%s' against path='%s'", pattern, path);
    ptok = strtok_r(pcopy, "/", &psave);
    utok = strtok_r(ucopy, "/", &usave);

    while (ptok && utok) {
        TRACE("Comparing ptok='%s' with utok='%s'", ptok, utok);
        
        if (ptok[0] == ':') {
            snprintf(out->params[out->count].key, sizeof(out->params[0].key),
                    "%s", ptok + 1);
            snprintf(out->params[out->count].value, sizeof(out->params[0].value),
                    "%s", utok);
            out->count++;
        } else if (strcmp(ptok, utok) != 0) {
            // printf("DEBUG: Mismatch! Returning 0\n");
            return 0;
        }
        ptok = strtok_r(NULL, "/", &psave);
        utok = strtok_r(NULL, "/", &usave);
    }

    int result = (ptok == NULL && utok == NULL);
    TRACE("Final result=%d (ptok=%p, utok=%p)", result, ptok, utok);
    return result;
}

// ------------------ Route Table ------------------

// static struct route routes[] = {
//     {"GET", "/users/:id", user_handler},
//     {"GET", "/posts/:postId/comments/:commentId", post_comment_handler},
// };


// #define ROUTE_COUNT (sizeof(routes) / sizeof(routes[0]))

// ------------------ Dispatcher ------------------

void dispatch_route(int c, http_request *req, struct route routes[], int n) {
  route_params params;

  // Remove trailing slash if present
  size_t len = strlen(req->url);
  if (len > 1 && req->url[len - 1] == '/') {
    ((char *)req->url)[len - 1] = 0;
  }

  // Static files
  if (strcmp(req->method, "GET") == 0 &&
      strncmp(req->url, "/static/", 8) == 0) {
    static_handler(c, req->url);
    return;
  }

  // Parameterized routes
  for (int i = 0; i < n; i++) {
    if (strcmp(req->method, routes[i].method) != 0)
      continue;

    if (match_route(routes[i].pattern, req->url, &params)) {
      routes[i].handler(c, &params, req);
      return;
    }
  }

  // Not found
  not_found(c, NULL, req);
}
