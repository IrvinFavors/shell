#include "watchuser.h"


void addUser(char *newUser){
  usersLL_t *temp, *newUsersLL = malloc(sizeof(struct usersLL));
  newUsersLL->user = malloc(strlen(newUser) + 1);
  strcpy(newUsersLL->user, newUser);
  newUsersLL->next = NULL;
  newUsersLL->prev = NULL;
  if(usersHead == NULL){
    usersHead = newUsersLL;
    usersHead->prev = NULL;
    usersHead->next = NULL;
  }
  else{
    temp = usersHead;
    while(temp->next != NULL) temp = temp->next;
    temp->next = newUsersLL;
    newUsersLL->prev = temp;
  }
}

void removeUser(char *user){
  usersLL_t *temp = usersHead;
  while (temp != NULL){
    if (strcmp(temp->user, user) == 0) {
      if (temp == usersHead) {
        usersHead = usersHead->next;
      }
      if (temp->prev != NULL) {
        temp->prev->next = temp->next;
      }
      if (temp->next != NULL) {
        temp->next->prev = temp->prev;
      }
      free(temp);
      return;
    }
    temp = temp->next;
  }
}

void *watchUser(void *arg){
  struct utmpx *ut;
  usersLL_t *temp = usersHead;
  while(1){ 
    setutxent();
    while(ut = getutxent()){
      if(ut->ut_type == USER_PROCESS){
        pthread_mutex_lock(&usersLock);
        while(temp != NULL){
          if(strncmp(temp->user, ut->ut_user, sizeof(ut->ut_user)) == 0) printf("%s has logged on %s from %s\n", ut->ut_user, ut->ut_line, ut->ut_host);
          temp = temp->next;
        }
        pthread_mutex_unlock(&usersLock); 
      }
      temp = usersHead;
    }
    sleep(20);
    fprintf(stdout, ">> ");
    fflush(stdout);
  }
}       
