
#include "./include/cframework.h"
// #include "./include/routes.h"
#include "include/http.h"
#include <stdio.h>

static void my_handler(http_request *req, http_response *res) {

  printf("the version: %s, url: %s\n", req->version, req->url);
  http_res_set_header(res, "Connection", "keep-alive");
  http_res_send(res, HTTP_OK, "text/plain", "my very nice");

  return;
}
static void other_handler(http_request *req, http_response *res) {
  printf("the version: %s, url: %s\n", req->version, req->url);
  http_res_send(res, HTTP_OK, "application/json", "{\"hh\": \"other very nice\"}");
}

int main() {
  // struct route routes[] = {{"GET", "/users/:id", my_handler},{"GET", "/boy/:ff", other_handler}};

  server_t srv = {0};
  srv.port = 3000;
  srv.server_name = "cfk.amizour.dz";
  srv.backlog = 128;

  cfk_add_get(&srv, "/hellokitty", my_handler);
  cfk_add_get(&srv, "/rahmano", other_handler);


  start_server(&srv);

  return 0;
}
