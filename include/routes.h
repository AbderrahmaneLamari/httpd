#ifndef ROUTES_H
#define ROUTES_H

#include "http.h"

#define MAX_PARAMS 8
#define MAX_ROUTE_COUNT 48

typedef struct {
  char key[32];
  char value[64];
} route_param;

typedef struct {
  route_param params[MAX_PARAMS];
  int count;
} route_params;

typedef void (*route_handler)(http_request *req, http_response *res);

struct route {
  const char *method;
  const char *pattern; // e.g. "/users/:id"
  route_handler handler;
};

void dispatch_route(int client_fd, http_request *req, struct route routes[], int n);

#endif