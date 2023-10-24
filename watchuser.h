#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <utmpx.h>

typedef struct usersLL{
  char *user;
  struct usersLL *next;
  struct usersLL *prev;
} usersLL_t;

usersLL_t *usersHead;
pthread_mutex_t usersLock;
int usersThread;
pthread_t thread;

void addUser(char *newUser);
void removeUser(char *user);
void *watchUser(void *arg);
