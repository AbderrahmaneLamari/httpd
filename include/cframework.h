#ifndef CFRAMEWORK
#define CFRAMEWORK

#include "routes.h"
int start_server(int port);
int set_routes(route_handler* routes);

static route_handler *route_handlers;




#endif