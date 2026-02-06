
#include "./include/cframework.h"
#include "./include/routes.h"
#include "include/http.h"
#include <stdio.h>

static void my_handler(int c, route_params *p, http_request *req) {

  printf("the version: %s, param: %s\n", req->version, p->params[0].value);

  return;
}
static void other_handler(int c, route_params *p, http_request *req){

}

int main() {
  struct route routes[] = {{"GET", "/users/:id", my_handler},{"GET", "/boy/:ff", other_handler}};


  start_server(3000, routes, sizeof(routes)/sizeof(routes[0]));


  return 0;
}
