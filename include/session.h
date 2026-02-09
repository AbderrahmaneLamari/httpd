#ifndef SESSION_H
#define SESSION_H

#include <time.h>
#include <stdint.h>
#include <pthread.h>

#define SESSION_ID_LENGTH 32
#define MAX_SESSIONS 1024
#define SESSION_TIMEOUT 3600  // 1 hour in seconds
#define MAX_SESSION_DATA 4096

typedef struct {
  char key[64];
  char value[256];
} session_pair;

typedef struct {
  char session_id[SESSION_ID_LENGTH + 1];
  time_t created_at;
  time_t last_accessed;
  
  session_pair data[32];  // Custom key-value pairs
  int data_count;
  
  int active;  // 1 if active, 0 if expired
} session_t;

typedef struct {
  session_t sessions[MAX_SESSIONS];
  int session_count;
  pthread_mutex_t lock;
} session_store_t;

// Core functions
session_store_t* session_store_init(void);
char* session_create(session_store_t *store, time_t now);
session_t* session_get(session_store_t *store, const char *session_id);
void session_set_data(session_t *session, const char *key, const char *value);
const char* session_get_data(session_t *session, const char *key);
void session_destroy(session_store_t *store, const char *session_id);
void session_cleanup_expired(session_store_t *store, time_t now);
void session_store_free(session_store_t *store);

#endif