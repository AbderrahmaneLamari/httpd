#ifndef USER_H
#define USER_H

#include "session.h"

typedef struct {
  char username[64];
  char password_hash[256];  // Use bcrypt in production!
  char email[128];
  char role[32];  // "user", "admin", etc.
  time_t created_at;
  int active;
} user_t;

typedef struct {
  user_t users[1024];
  int user_count;
  pthread_mutex_t lock;
} user_store_t;

// User management functions
user_store_t* user_store_init(void);
user_t* user_authenticate(user_store_t *store, const char *username, const char *password);
user_t* user_get_by_username(user_store_t *store, const char *username);
int user_create(user_store_t *store, const char *username, const char *password, const char *email);
void user_store_free(user_store_t *store);

#endif