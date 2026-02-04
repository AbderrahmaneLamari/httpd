#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "../include/server.h"

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }

    int s = server_init(atoi(argv[1]));
    printf("Listening on port %s\n", argv[1]);

    while (1) {
        int c = server_accept(s);
        if (!fork()) {
            close(s);
            handle_client(s, c);
            close(c);
            exit(0);
        }
        close(c);
    }
}
