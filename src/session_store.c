
#include "../include/session_store.h"
#include "../include/clog.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int session_save_to_file(session_store_t *store, const char *filename) {
  pthread_mutex_lock(&store->lock);
  
  FILE *f = fopen(filename, "wb");
  if (!f) {
    pthread_mutex_unlock(&store->lock);
    ERROR("Failed to open session file for writing: %s", filename);
    return -1;
  }

  // Write session count
  fwrite(&store->session_count, sizeof(int), 1, f);

  // Write each active session
  for (int i = 0; i < store->session_count; i++) {
    if (store->sessions[i].active) {
      fwrite(&store->sessions[i], sizeof(session_t), 1, f);
    }
  }

  fclose(f);
  pthread_mutex_unlock(&store->lock);
  
  DEBUG("Saved %d sessions to %s", store->session_count, filename);
  return 0;
}

int session_load_from_file(session_store_t *store, const char *filename) {
  pthread_mutex_lock(&store->lock);
  
  FILE *f = fopen(filename, "rb");
  if (!f) {
    pthread_mutex_unlock(&store->lock);
    DEBUG("No session file found: %s (this is normal on first run)", filename);
    return 0;
  }

  int count = 0;
  fread(&count, sizeof(int), 1, f);

  for (int i = 0; i < count && store->session_count < MAX_SESSIONS; i++) {
    session_t session;
    if (fread(&session, sizeof(session_t), 1, f) == 1) {
      // Check if session hasn't expired
      if (time(NULL) - session.last_accessed < SESSION_TIMEOUT) {
        store->sessions[store->session_count++] = session;
      }
    }
  }

  fclose(f);
  pthread_mutex_unlock(&store->lock);
  
  DEBUG("Loaded %d sessions from %s", store->session_count, filename);
  return 0;
}

session_persistence_t* session_persistence_init(const char *filepath, int interval) {
  session_persistence_t *persist = (session_persistence_t *)malloc(sizeof(session_persistence_t));
  if (!persist) return NULL;
  
  strncpy(persist->filepath, filepath, sizeof(persist->filepath) - 1);
  persist->auto_save_interval = interval;
  persist->last_save = time(NULL);
  
  return persist;
}

int session_persist(session_store_t *store, session_persistence_t *persist) {
  time_t now = time(NULL);
  
  if (now - persist->last_save >= persist->auto_save_interval) {
    int result = session_save_to_file(store, persist->filepath);
    if (result == 0) {
      persist->last_save = now;
    }
    return result;
  }
  
  return 0;
}

void session_persistence_free(session_persistence_t *persist) {
  if (persist) {
    free(persist);
  }
}