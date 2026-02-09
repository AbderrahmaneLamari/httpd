#include "../include/middleware.h"
#include "../include/clog.h"
#include <string.h>

int require_session(http_request *req, http_response *res) {
  if (!req->session) {
    http_res_send(res, 401, "application/json", 
      "{\"error\": \"No session\"}");
    return 0;  // Fail
  }
  return 1;  // Pass
}

int require_login(http_request *req, http_response *res) {
  if (!req->session) {
    http_res_send(res, 401, "application/json", 
      "{\"error\": \"Not authenticated\"}");
    return 0;
  }
  
  const char *logged_in = session_get_data(req->session, "logged_in");
  if (!logged_in || strcmp(logged_in, "1") != 0) {
    http_res_send(res, 401, "application/json", 
      "{\"error\": \"Not logged in\"}");
    return 0;
  }
  
  return 1;  // Pass
}

int require_admin(http_request *req, http_response *res) {
  // First check if logged in
  if (!require_login(req, res)) {
    return 0;
  }
  
  const char *role = session_get_data(req->session, "role");
  if (!role || strcmp(role, "admin") != 0) {
    http_res_send(res, 403, "application/json", 
      "{\"error\": \"Admin privileges required\"}");
    return 0;
  }
  
  return 1;  // Pass
}

int cors_middleware(http_request *req, http_response *res) {
  http_res_set_header(res, "Access-Control-Allow-Origin", "*");
  http_res_set_header(res, "Access-Control-Allow-Methods", 
    "GET, POST, PUT, DELETE, OPTIONS");
  http_res_set_header(res, "Access-Control-Allow-Headers", "Content-Type");
  DEBUG("Executing cors middleware");
  
  return 1;  // Always pass
}