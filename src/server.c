#include "../include/server.h"
#include "../include/clog.h"
#include "../include/http.h"
#include "../include/routes.h"
#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int server_init(int port) {
  int s = socket(AF_INET, SOCK_STREAM, 0);

  struct sockaddr_in addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(port);

  int one = 1;
  setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

  int bind_res = bind(s, (struct sockaddr *)&addr, sizeof(addr));
  if (bind_res) {
    fprintf(stderr, "bind() error");
    exit(1);
  }
  listen(s, 10);

  return s;
}

int server_accept(int s) {

  // client socket address
  struct sockaddr_in cli;
  socklen_t len = sizeof(cli);

  return accept(s, (struct sockaddr *)&cli, &len);
}

// Thread pool logic

static void *worker_loop(void *arg) {
  thread_pool_t *pool = arg;

  while (1) {
    pthread_mutex_lock(&pool->lock);

    while (pool->queue_head == pool->queue_tail)
      pthread_cond_wait(&pool->cond, &pool->lock);

    job_t job = pool->queue[pool->queue_head];
    pool->queue_head = (pool->queue_head + 1) % pool->queue_size;

    pthread_mutex_unlock(&pool->lock);

    handle_connection(job.conn, job.routes, job.n_routes, job.session_store);
  }
  return NULL;
}

void pool_submit(thread_pool_t *pool, job_t job) {
  pthread_mutex_lock(&pool->lock);

  pool->queue[pool->queue_tail] = job;
  pool->queue_tail = (pool->queue_tail + 1) % pool->queue_size;

  pthread_cond_signal(&pool->cond);
  pthread_mutex_unlock(&pool->lock);
}

void handle_connection(connection_t *conn, struct route routes[], int n,  session_store_t *session_store) {
  DEBUG("=== handle_connection START: fd=%d ===", conn->fd);

  http_request req;
  memset(&req, 0, sizeof(req));

  DEBUG("Before http_read_request: req.url='%s', req.method='%s'", req.url,
        req.method);

  int r = http_read_request(conn->fd, &req);

  DEBUG("After http_read_request: r=%d, req.url='%s', req.method='%s'", r,
        req.url, req.method);

  if (r == -2) {
    conn->want_read = 1;
    DEBUG("=== handle_connection END: EAGAIN ===");
    return;
  }

  if (r == 0) {
    ERROR("Client closed connection");
    close(conn->fd);
    conn->closed = 1;
    DEBUG("=== handle_connection END: closed ===");
    return;
  }

  if (r < 0) {
    if (errno == ECONNRESET || errno == EPIPE) {
      // Client closed connection abruptly - this is normal
      DEBUG("Client closed connection");
    } else {
      ERROR("Error parsing HTTP request: errno=%d", errno);
    }
    http_response res;
    http_res_init(&res, conn->fd);
    http_res_send(&res, 400, "text/plain", "Bad Request");
    close(conn->fd);
    conn->closed = 1;
    DEBUG("=== handle_connection END: error ===");
    return;
  }

  // Success - r contains number of bytes read
  conn->read_len += r;

  DEBUG("About to call dispatch_route: req.url='%s', req.method='%s'", req.url,
        req.method);
  DEBUG("req address: %p", (void *)&req);

  // Retrieve or create session
  if (req.session_id[0] != '\0') {
    req.session = session_get(session_store, req.session_id);
    if (!req.session) {
      DEBUG("Session ID provided but not found, creating new one");
      char *new_id = session_create(session_store, time(NULL));
      strcpy(req.session_id, new_id);
      req.session = session_get(session_store, new_id);
    }
  } else {
    // No session ID provided, create new one
    char *new_id = session_create(session_store, time(NULL));
    strcpy(req.session_id, new_id);
    req.session = session_get(session_store, new_id);
  }

  // Process the request
  dispatch_route(conn->fd, &req, routes, n);

  DEBUG("=== handle_connection END: success ===");

  conn->keep_alive = req.keep_alive;
  conn->read_len = 0;

  if (!conn->keep_alive) {
    close(conn->fd);
    conn->closed = 1;
  }
}

int set_nonblocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1)
    return -1;

  return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}
int pool_init(thread_pool_t *pool, int threads) {
  DEBUG("Initializing thread pool");

  pool->threads = calloc(threads, sizeof(pthread_t));
  DEBUG("Thread count: %d", threads);
  pool->thread_count = threads; // Store thread count
  pool->queue_size = 64;
  pool->queue = calloc(pool->queue_size, sizeof(job_t));
  pool->queue_head = 0;
  pool->queue_tail = 0;
  pthread_mutex_init(&pool->lock, NULL);
  pthread_cond_init(&pool->cond, NULL);

  for (int i = 0; i < threads; i++) {
    pthread_create(&pool->threads[i], NULL, worker_loop, pool);
  }

  return 0;
}
