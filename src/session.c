#include "../include/session.h"
#include "../include/clog.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>


session_store_t g_session_store = {
    .lock = PTHREAD_MUTEX_INITIALIZER,
    .session_count = 0
};

// Simple session ID generator (not cryptographically secure, use libsodium in production)
static void generate_session_id(char *id, size_t len) {
  const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  for (size_t i = 0; i < len - 1; i++) {
    int idx = rand() % (sizeof(charset) - 1);
    id[i] = charset[idx];
  }
  id[len - 1] = '\0';
}

session_store_t* session_store_init(void) {
  // session_store_t *store = calloc(1, sizeof(session_store_t));
  // if (!store) return NULL;
  
  pthread_mutex_init(&g_session_store.lock, NULL);
  g_session_store.session_count = 0;
  
  DEBUG("Session store initialized");
  return &g_session_store;
}

char* session_create(session_store_t *store, time_t now) {
  pthread_mutex_lock(&store->lock);
  
  if (store->session_count >= MAX_SESSIONS) {
    WARN("Session store is full, cannot create new session");
    pthread_mutex_unlock(&store->lock);
    return NULL;
  }
  
  session_t *session = &store->sessions[store->session_count];
  memset(session, 0, sizeof(session_t));
  
  generate_session_id(session->session_id, SESSION_ID_LENGTH + 1);
  session->created_at = now;
  session->last_accessed = now;
  session->active = 1;
  session->data_count = 0;
  
  store->session_count++;
  
  DEBUG("Created session: %s", session->session_id);
  
  pthread_mutex_unlock(&store->lock);
  
  return session->session_id;
}

session_t* session_get(session_store_t *store, const char *session_id) {
  pthread_mutex_lock(&store->lock);
  
  for (int i = 0; i < store->session_count; i++) {
    if (store->sessions[i].active && 
        strcmp(store->sessions[i].session_id, session_id) == 0) {
      
      // Update last accessed time
      store->sessions[i].last_accessed = time(NULL);
      
      pthread_mutex_unlock(&store->lock);
      return &store->sessions[i];
    }
  }
  
  pthread_mutex_unlock(&store->lock);
  return NULL;
}

void session_set_data(session_t *session, const char *key, const char *value) {
  if (!session || session->data_count >= 32) return;
  
  // Check if key already exists
  for (int i = 0; i < session->data_count; i++) {
    if (strcmp(session->data[i].key, key) == 0) {
      strncpy(session->data[i].value, value, sizeof(session->data[i].value) - 1);
      session->data[i].value[sizeof(session->data[i].value) - 1] = '\0';
      return;
    }
  }
  
  // Add new key-value pair
  snprintf(session->data[session->data_count].key, sizeof(session->data[0].key), "%s", key);
  snprintf(session->data[session->data_count].value, sizeof(session->data[0].value), "%s", value);
  session->data_count++;
}

const char* session_get_data(session_t *session, const char *key) {
  if (!session) return NULL;
  
  for (int i = 0; i < session->data_count; i++) {
    if (strcmp(session->data[i].key, key) == 0) {
      return session->data[i].value;
    }
  }
  
  return NULL;
}

void session_destroy(session_store_t *store, const char *session_id) {
  pthread_mutex_lock(&store->lock);
  
  for (int i = 0; i < store->session_count; i++) {
    if (strcmp(store->sessions[i].session_id, session_id) == 0) {
      store->sessions[i].active = 0;
      DEBUG("Destroyed session: %s", session_id);
      break;
    }
  }
  
  pthread_mutex_unlock(&store->lock);
}

void session_cleanup_expired(session_store_t *store, time_t now) {
  pthread_mutex_lock(&store->lock);
  
  int expired_count = 0;
  for (int i = 0; i < store->session_count; i++) {
    if (store->sessions[i].active) {
      time_t age = now - store->sessions[i].last_accessed;
      if (age > SESSION_TIMEOUT) {
        store->sessions[i].active = 0;
        expired_count++;
      }
    }
  }
  
  if (expired_count > 0) {
    DEBUG("Cleaned up %d expired sessions", expired_count);
  }
  
  pthread_mutex_unlock(&store->lock);
}

void session_store_free(session_store_t *store) {
  if (store) {
    pthread_mutex_destroy(&store->lock);
    free(store);
  }
}