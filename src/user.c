#include "../include/user.h"
#include "../include/clog.h"
#include <string.h>
#include <stdlib.h>

// NOTE: In production, use bcrypt or argon2!
// This is just a placeholder - DO NOT use in production
static int simple_hash_match(const char *password, const char *hash) {
  // This is a terrible implementation - just for demo
  // Real implementation should use bcrypt: bcrypt(password, salt)
  return strcmp(password, hash) == 0;
}

user_store_t* user_store_init(void) {
  user_store_t *store = calloc(1, sizeof(user_store_t));
  if (!store) return NULL;
  
  pthread_mutex_init(&store->lock, NULL);
  
  // Add a default user for testing
  user_create(store, "admin", "password123", "admin@example.com");
  
  return store;
}

user_t* user_authenticate(user_store_t *store, const char *username, const char *password) {
  pthread_mutex_lock(&store->lock);
  
  for (int i = 0; i < store->user_count; i++) {
    if (strcmp(store->users[i].username, username) == 0 && store->users[i].active) {
      if (simple_hash_match(password, store->users[i].password_hash)) {
        pthread_mutex_unlock(&store->lock);
        return &store->users[i];
      }
      break;
    }
  }
  
  pthread_mutex_unlock(&store->lock);
  return NULL;
}

user_t* user_get_by_username(user_store_t *store, const char *username) {
  pthread_mutex_lock(&store->lock);
  
  for (int i = 0; i < store->user_count; i++) {
    if (strcmp(store->users[i].username, username) == 0 && store->users[i].active) {
      pthread_mutex_unlock(&store->lock);
      return &store->users[i];
    }
  }
  
  pthread_mutex_unlock(&store->lock);
  return NULL;
}

int user_create(user_store_t *store, const char *username, const char *password, const char *email) {
  pthread_mutex_lock(&store->lock);
  
  if (store->user_count >= 1024) {
    pthread_mutex_unlock(&store->lock);
    return -1;
  }
  
  user_t *user = &store->users[store->user_count];
  memset(user, 0, sizeof(user_t));
  
  strncpy(user->username, username, sizeof(user->username) - 1);
  strncpy(user->password_hash, password, sizeof(user->password_hash) - 1);  // NEVER DO THIS IN PRODUCTION
  strncpy(user->email, email, sizeof(user->email) - 1);
  strncpy(user->role, "user", sizeof(user->role) - 1);
  user->created_at = time(NULL);
  user->active = 1;
  
  store->user_count++;
  
  DEBUG("Created user: %s", username);
  
  pthread_mutex_unlock(&store->lock);
  return 0;
}

void user_store_free(user_store_t *store) {
  if (store) {
    pthread_mutex_destroy(&store->lock);
    free(store);
  }
}