#ifndef SESSION_STORE_H
#define SESSION_STORE_H

#include "session.h"

// File-based session persistence
int session_save_to_file(session_store_t *store, const char *filename);
int session_load_from_file(session_store_t *store, const char *filename);

// Redis-like persistence (simplified)
typedef struct {
  char filepath[256];
  int auto_save_interval;  // seconds
  time_t last_save;
} session_persistence_t;

session_persistence_t* session_persistence_init(const char *filepath, int interval);
int session_persist(session_store_t *store, session_persistence_t *persist);
void session_persistence_free(session_persistence_t *persist);

#endif