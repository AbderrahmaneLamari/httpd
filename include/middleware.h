#ifndef MIDDLEWARE_H
#define MIDDLEWARE_H
#define MAX_MIDDLEWARES 16
#include "http.h"

// Middleware function pointer type
typedef int (*middleware_fn)(http_request *req, http_response *res);

// Common middleware functions
int require_session(http_request *req, http_response *res);
int require_login(http_request *req, http_response *res);
int require_admin(http_request *req, http_response *res);
int cors_middleware(http_request *req, http_response *res);

#endif