#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "../include/server.h"
#include "../include/http.h"
#include "../include/routes.h"

int server_init(int port) {
    int s = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    int bind_res = bind(s, (struct sockaddr *)&addr, sizeof(addr));
    if(bind_res){
        fprintf(stderr, "bind() error");
        exit(1);
    }
    listen(s, 10);

    return s;
}

int server_accept(int s) {
    struct sockaddr_in cli;
    socklen_t len = sizeof(cli);
    return accept(s, (struct sockaddr *)&cli, &len);
}

void handle_client(int s, int c) {
    char *raw = cli_read(c);
    if (!raw) return;

    httpreq *req = parse_http(raw);
    if (!req) return;

    dispatch_route(c, req);
    free(req);
}
