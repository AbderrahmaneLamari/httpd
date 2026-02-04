#include "../include/routes.h"
#include "../include/http.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STATIC_ROOT "./www"

// ------------------ Handlers ------------------

static void user_handler(int c, route_params *p) {
  char body[128];
  snprintf(body, sizeof(body), "User ID = %s\n", p->params[0].value);
  http_response(c, 200, "text/plain", body);
}

static void post_comment_handler(int c, route_params *p) {
  char body[256];
  snprintf(body, sizeof(body), "Post %s, Comment %s\n", p->params[0].value,
           p->params[1].value);
  http_response(c, 200, "text/plain", body);
}

static void not_found(int c, route_params *p) {
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

struct route {
  const char *method;
  const char *pattern; // e.g. "/users/:id"
  route_handler handler;
};

static int match_route(const char *pattern, const char *path,
                      route_params *out) {
    char pcopy[256], ucopy[256];
    char *ptok, *utok;
    char *psave, *usave;
    out->count = 0;

    snprintf(pcopy, sizeof(pcopy), "%s", pattern);
    snprintf(ucopy, sizeof(ucopy), "%s", path);

    printf("DEBUG: Matching pattern='%s' against path='%s'\n", pattern, path);

    ptok = strtok_r(pcopy, "/", &psave);
    utok = strtok_r(ucopy, "/", &usave);

    while (ptok && utok) {
        printf("DEBUG: Comparing ptok='%s' with utok='%s'\n", ptok, utok);
        
        if (ptok[0] == ':') {
            snprintf(out->params[out->count].key, sizeof(out->params[0].key),
                    "%s", ptok + 1);
            snprintf(out->params[out->count].value, sizeof(out->params[0].value),
                    "%s", utok);
            out->count++;
        } else if (strcmp(ptok, utok) != 0) {
            printf("DEBUG: Mismatch! Returning 0\n");
            return 0;
        }
        ptok = strtok_r(NULL, "/", &psave);
        utok = strtok_r(NULL, "/", &usave);
    }

    int result = (ptok == NULL && utok == NULL);
    printf("DEBUG: Final result=%d (ptok=%p, utok=%p)\n", result, ptok, utok);
    return result;
}

// ------------------ Route Table ------------------

static struct route routes[] = {
    {"GET", "/users/:id", user_handler},
    {"GET", "/posts/:postId/comments/:commentId", post_comment_handler},
};

#define ROUTE_COUNT (sizeof(routes) / sizeof(routes[0]))

// ------------------ Dispatcher ------------------

void dispatch_route(int c, httpreq *req) {
  route_params params;

  // Remove trailing slash if present
  size_t len = strlen(req->url);
  if (len > 1 && req->url[len - 1] == '/') {
    ((char *)req->url)[len - 1] = 0;
  }

  // 1️⃣ Static files
  if (strcmp(req->method, "GET") == 0 &&
      strncmp(req->url, "/static/", 8) == 0) {
    static_handler(c, req->url);
    return;
  }

  // 2️⃣ Parameterized routes
  for (int i = 0; i < ROUTE_COUNT; i++) {
    if (strcmp(req->method, routes[i].method) != 0)
      continue;

    if (match_route(routes[i].pattern, req->url, &params)) {
      routes[i].handler(c, &params);
      return;
    }
  }

  // 3️⃣ Not found
  not_found(c, NULL);
}
