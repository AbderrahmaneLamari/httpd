#ifndef SERVER_H
#define SERVER_H

int server_init(int port);
int server_accept(int server_fd);
void handle_client(int server_fd, int client_fd);

#endif
