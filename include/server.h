#ifndef SERVER_H
#define SERVER_H

#include "./routes.h"
#include <pthread.h>
#include <stddef.h>
#include <unistd.h>

typedef struct {
  int fd;
  int keep_alive;
  int want_read;
  int closed;
  char read_buf[8192];
  size_t read_len;
} connection_t;

typedef struct {
  connection_t *conn;
  struct route *routes;
  int n_routes;
} job_t;

typedef struct {
  pthread_t *threads;
  int thread_count;

  job_t *queue;
  int queue_size;
  int queue_head;
  int queue_tail;

  pthread_mutex_t lock;
  pthread_cond_t cond;
} thread_pool_t;

int server_init(int port);
int server_accept(int server_fd);
void handle_connection(connection_t *conn, struct route routes[], int n);
int set_nonblocking(int fd);
void pool_submit(thread_pool_t *pool, job_t job);
int pool_init(thread_pool_t *pool, int threads);
#endif
