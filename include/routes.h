#ifndef ROUTES_H
#define ROUTES_H

#include "http.h"

#define MAX_PARAMS 8

typedef struct {
  char key[32];
  char value[64];
} route_param;

typedef struct {
  route_param params[MAX_PARAMS];
  int count;
} route_params;

typedef void (*route_handler)(int client_fd, route_params *params, http_request *req);

struct route {
  const char *method;
  const char *pattern; // e.g. "/users/:id"
  route_handler handler;
};

void dispatch_route(int client_fd, http_request *req);

#endif