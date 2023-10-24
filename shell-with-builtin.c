#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <glob.h>
#include <sys/wait.h>
#include <pthread.h>
#include "sh.h"
#include "watchuser.h"

void sig_handler(int sig)
{
  fprintf(stdout, "\n>> ");
  fflush(stdout);
}
  
int
main(int argc, char **argv, char **envp)
{
	char	buf[MAXLINE];
	char    *arg[MAXARGS];  // an array of tokens
	char    *ptr;
        char    *pch;
	pid_t	pid;
	int	status, i, arg_no, background, noclobber;
	int     redirection, append, ispipe, rstdin, rstdout, rstderr;
        struct pathelement *dir_list, *tmp;
        char *cmd;

        noclobber = 0;         // initially default to 0

        signal(SIGINT, sig_handler);

	fprintf(stdout, ">> ");	/* print prompt (printf requires %% to print %) */
	fflush(stdout);
	while (fgets(buf, MAXLINE, stdin) != NULL) {
		if (strlen(buf) == 1 && buf[strlen(buf) - 1] == '\n')
		  goto nextprompt;  // "empty" command line

		if (buf[strlen(buf) - 1] == '\n')
			buf[strlen(buf) - 1] = 0; /* replace newline with null */

	                       // no redirection or pipe
	        redirection = append = ispipe = rstdin = rstdout = rstderr = 0; 
                // check for >, >&, >>, >>&, <, |, and |&
		if (strstr(buf, ">>&")) 
		  redirection = append = rstdout = rstderr = 1;
		else
		if (strstr(buf, ">>")) 
		  redirection = append = rstdout = 1;
		else
		if (strstr(buf, ">&")) 
		  redirection = rstdout = rstderr = 1;
		else
		if (strstr(buf, ">")) 
		  redirection = rstdout = 1;
		else
		if (strstr(buf, "<")) 
		  redirection = rstdin = 1;
		else
		if (strstr(buf, "|&")) 
		  ispipe = rstdout = rstderr = 1;
		else
		if (strstr(buf, "|")) 
		  ispipe = rstdout = 1;

        	/* 
                printf("-%d-%d-%d-%d-%d-%d-\n",
		       redirection, append, pipe, rstdin, rstdout, rstderr);
       		*/

		// parse command line into tokens (stored in buf)
		arg_no = 0;
                pch = strtok(buf, " ");
                while (pch != NULL && arg_no < MAXARGS)
                {
		  arg[arg_no] = pch;
		  arg_no++;
                  pch = strtok (NULL, " ");
                }
		arg[arg_no] = (char *) NULL;

		if (arg[0] == NULL)  // "blank" command line
		  goto nextprompt;

                background = 0;      // not background process
		if (arg[arg_no-1][0] == '&')
		  background = 1;    // to background this command
                  
		/***
		for (i = 0; i < arg_no; i++)
		  printf("arg[%d] = %s\n", i, arg[i]);
           	 ***/

                if (strcmp(arg[0], "exit") == 0) { // built-in command exit
		  printf("Executing built-in [exit]\n");
                  exit(0);
	        }
		else
                if (strcmp(arg[0], "cd") == 0) { // built-in command cd
		  printf("Executing built-in [cd]\n");
		  char *pa;
		  if (arg_no == 1)         // cd
		    pa = getenv("HOME");
		  else
		    pa = arg[1];           // cd arg[1]
                  if (chdir(pa) < 0)
		    printf("%s: No such file or directory.\n", pa);
		  goto nextprompt;
	        }

		if (redirection) goto external;
		if (ispipe) goto external;


		else
                if (strcmp(arg[0], "pwd") == 0) { // built-in command pwd 
		  printf("Executing built-in [pwd]\n");
	          ptr = getcwd(NULL, 0);
                  if (redirection) {
		    int fid;
		    if (!append && rstdout && !rstderr)  // ">"
                      fid = open(arg[arg_no-1], O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP);
                    else
                    if (append && rstdout && !rstderr)   // ">>"
                      fid = open(arg[arg_no-1], O_WRONLY|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR|S_IRGRP);

                    close(1);
                    dup(fid);
                    close(fid);
                    printf("%s\n", ptr);  // redirect stdout to file
		    fid = open("/dev/tty", O_WRONLY); // redirect stdout
                    close(1);                         // back to terminal
                    dup(fid);
                    close(fid);
                  } else
                    printf("%s\n", ptr);  // no redirection
                  free(ptr);
	        }
		else
                if (strcmp(arg[0], "noclobber") == 0) { // built-in command noclobber
		  printf("Executing built-in [noclobber]\n");
		  noclobber = 1 - noclobber; // switch value
		  printf("%d\n", noclobber);
	        }
		else
                if (strcmp(arg[0], "echo") == 0) { // built-in command echo
		  printf("Executing built-in [echo]\n");
                /***
		  for (i = 0; i < arg_no; i++)
		    printf("arg[%d] = %s\n", i, arg[i]);
                ***/
                  printf("%s\n", arg[1]);
	        }
		else
                if (strcmp(arg[0], "which") == 0) { // built-in command which
		  printf("Executing built-in [which]\n");

		  if (arg[1] == NULL) {  // "empty" which
		    printf("which: Too few arguments.\n");
		    goto nextprompt;
                  }

		  dir_list = get_path();

                  cmd = which(arg[1], dir_list);
                  if (cmd) {
		    printf("%s\n", cmd);
                    free(cmd);
                  }
		  else               // argument not found
		    printf("%s: Command not found\n", arg[1]);

		  while (dir_list) {   // free list of path values
		     tmp = dir_list;
		     dir_list = dir_list->next;
		     free(tmp->element);
		     free(tmp);
                  }
	        }

                
                else if (strcmp(arg[0], "watchuser") == 0) { // built-in command watchuser
      	           if(arg_no == 2){
                      printf("Executing built-in %s\n", arg[0]);
        	      if(usersThread == 0){ //creates a thread of users
                         pthread_create(&thread, NULL, watchUser, "List Of Users");
                         usersThread = 1;
                      }
                      pthread_mutex_lock(&usersLock); //locks users thread
        	      addUser(arg[1]); //adds user to the list
        	      pthread_mutex_unlock(&usersLock); //unlocks users thread
      	           }
      	           else if(arg_no == 3 && strcmp(arg[2], "off") == 0){
        	      printf("Executing built-in %s\n", arg[0]);
        	      pthread_mutex_lock(&usersLock);
        	      removeUser(arg[1]);
        	      pthread_mutex_unlock(&usersLock);
      	           }
      		   else{
	             printf("error\n");
      	           }
		   sleep(1);
		   goto nextprompt;
		}


                // add other built-in commands here
		else {  // external command
	external:
		  if ((pid = fork()) < 0) {
			printf("fork error");
		  } else if (pid == 0) {		/* child */
			                // an array of aguments for execve()
	                char    *execargs[MAXARGS]; 
		        glob_t  paths;
                        int     csource, j;
			char    **p;

			printf("%d\n", strncmp(arg[0], "./", 2) != 0);

                        if (arg[0][0] != '/' && strncmp(arg[0], "./", 2) != 0 && strncmp(arg[0], "../", 3) != 0) {  // get absoulte path of command
		          dir_list = get_path();      
                          cmd = which(arg[0], dir_list);
                          if (cmd) 
		            printf("Executing [%s]\n", cmd);
		          else {              // argument not found
		            printf("%s: Command not found\n", arg[1]);
			    goto nextprompt;
                          }
		          while (dir_list) {   // free list of path values
		             tmp = dir_list;
		             dir_list = dir_list->next;
		             free(tmp->element);
		             free(tmp);
                          }
			  execargs[0] = malloc(strlen(cmd)+1);
			  strcpy(execargs[0], cmd); // copy "absolute path"
			  free(cmd);
			}
			else {
			  execargs[0] = malloc(strlen(arg[0])+1);
			  strcpy(execargs[0], arg[0]); // copy "command"
                        }
		        j = 1;
		        for (i = 1; i < arg_no; i++) { // check arguments
			  if (strchr(arg[i], '*') != NULL) { // wildcard!
			    csource = glob(arg[i], 0, NULL, &paths);
                            if (csource == 0) {
                              for (p = paths.gl_pathv; *p != NULL; ++p) {
                                execargs[j] = malloc(strlen(*p)+1);
				strcpy(execargs[j], *p);
				j++;
                              }
                              globfree(&paths);
                            }
			    else
                            if (csource == GLOB_NOMATCH) {
                              execargs[j] = malloc(strlen(arg[i])+1);
			      strcpy(execargs[j], arg[i]);
			      j++;
			    }
                          }
			  else {
                            execargs[j] = malloc(strlen(arg[i])+1);
			    strcpy(execargs[j], arg[i]);
			    j++;
			  }
                        }
                        execargs[j] = NULL;

			if (background) { // get rid of argument "&"
			  j--;
			  free(execargs[j]);
                          execargs[j] = NULL;
                        }

		        // print arguments into execve()
                     /***
		        for (i = 0; i < j; i++)
		          printf("exec arg[%d] = %s\n", i, execargs[i]);
                     ***/
		
			if (redirection){
	                  int redirectIndex = -1;
			  for (int i = 0; i < arg_no; i++){
			     if ((strcmp(execargs[i], ">>")) == 0 
			       ||(strcmp(execargs[i], ">")) == 0
			       ||(strcmp(execargs[i], ">&")) == 0
			       ||(strcmp(execargs[i], ">>&")) == 0
			       ||(strcmp(execargs[i], "<")) == 0) redirectIndex = i;
			  }
			  //printf("Index: %d\n", redirectIndex);
			  int fid, fileExists = 0;
			  struct stat status;

			  if (stat(arg[arg_no-1], &status) == 0) {
				  printf("%s: File exists\n", arg[arg_no-1]);
				  fileExists = 1;
			  }else {
			     printf("%s: No such file or directory\n", arg[arg_no-1]);
			  }

                          if (!append && rstdout && !rstderr)  // ">"
		            if (noclobber == 1 && fileExists == 1) printf("Cannot overwrite with noclobber on!\n");
			    else { 
                            	fid = open(arg[arg_no-1], O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP);
			        execargs[redirectIndex] = NULL;
		                dup2(fid, STDOUT_FILENO);
				close(fid);
				execve(execargs[0], execargs, NULL);
			    }
                          else
                            if (append && rstdout && !rstderr) {   // ">>"
		              if (noclobber == 1 && fileExists == 0) 
				 printf("%s does not exits. noclobber being on prevents fils creation\n", arg[arg_no-1]);
			      else {
				 fid = open(arg[arg_no-1], O_WRONLY|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR|S_IRGRP);
				 execargs[redirectIndex] = NULL;
				 dup2(fid, STDOUT_FILENO);
				 close(fid);
				 execve(execargs[0], execargs, NULL);
			      }
			    }
			  else if (!append && rstdout && rstderr) { // ">&"
			      if (noclobber == 1 && fileExists == 1) 
				 printf("Cannot overwrite %s with no clobber on\n", arg[arg_no-1]);
			      else {
		                fid = open(arg[arg_no-1], O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP);
				execargs[redirectIndex] = NULL;
				dup2(fid, STDOUT_FILENO);
			        dup2(fid, STDERR_FILENO);
				close(fid);
				execve(execargs[0], execargs, NULL);
			      }
			  }
			  else if (append && rstdout && rstderr) { // ">>&"
		              if (noclobber == 1 && fileExists == 1)
				 printf("%s does not exits. noclobber being on prevents fils creation\n", arg[arg_no-1]);
			      else {
                                fid = open(arg[arg_no-1], O_WRONLY|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR|S_IRGRP);
				execargs[redirectIndex] = NULL;
				dup2(fid, STDOUT_FILENO);
				dup2(fid, STDERR_FILENO);
				close(fid);
				execve(execargs[0], execargs, NULL);
			      }
			  }
			  else if (!append && !rstdout && !rstderr && rstdin) { // "<"
                              if (stat(arg[arg_no-1], &status) == -1) printf("Error opening %s\n", arg[arg_no-1]);
			      else {
		                fid = open(arg[arg_no-1], O_RDONLY);
				execargs[redirectIndex] = NULL;
				dup2(fid, STDIN_FILENO);
				close(fid);
				execve(execargs[0], execargs, NULL);
			      }
			  }
			  fid = open("/dev/tty", O_WRONLY);
			  dup2(fid, STDOUT_FILENO);
			  close(fid);
			  fileExists = 0;
			}
			else if (ispipe) {
			  //printf("pipe\n");
			  int pipeIndex = -1;
                          for (int i = 0; i < arg_no; i++){
                             if ((strcmp(execargs[i], "|")) == 0 || 
			         (strcmp(execargs[i], "|&")) == 0) pipeIndex = i;
                          }
                          printf("Index: %d\n", pipeIndex);
			  

			  int fds[2];
			  int pid1 = -1, pid2 = -1, pipe_args_no = 0;
			  char **pipe_args = calloc(MAXARGS, sizeof(char *));

			  for (int i = 0; i < MAXARGS; i++) {
			    pipe_args[i] = NULL;
			  }
			  execargs[pipeIndex] = NULL; //break pipe into two commands

			  for (int i = pipeIndex+1; i < MAXARGS; i++) {
			    if(execargs[i] != NULL) {
			      pipe_args[i-pipeIndex-1] = execargs[i];
			      pipe_args_no++;
			    }else break;
			  }

			  for(int i = pipeIndex; i < MAXARGS; i++) {
			    if (execargs[i] == NULL) break;
			    execargs[i] = NULL;
			    arg_no--;
			  }

			  //for (int i = 0; i < pipe_args_no; i++) printf("%s\n", pipe_args[i]);

			  pipe(fds);

			  if (rstdout && ! rstderr){ // "|"
			    pid1 = fork();
			    if (pid1 == 0){
                    	     dup2(fds[1], STDOUT_FILENO);
			     close(fds[0]);
                    	     close(fds[1]);
			     execvp(execargs[0], execargs);
			     //execve(execargs[0], execargs, NULL);

			     exit(0);

			    } else {
			        pid2 = fork();

                    	        if(pid2 == 0){

                                dup2(fds[0], STDIN_FILENO);
			        close(fds[1]);
                                close(fds[0]);
			        execvp(pipe_args[0], pipe_args);
			      //execve(pipe_args[0], pipe_args, NULL);
                        
                                exit(0);
                                }else{
                                   close(fds[0]);
                                   close(fds[1]);
                                   waitpid(pid1, NULL, WNOHANG);
				   waitpid(pid2, NULL, WNOHANG);
				   sleep(1);

                                   for (int i = 0; i < MAXARGS; i++) {
                                      if(pipe_args[i] != NULL){
                                      free(pipe_args[i]);
                                   }
				   exit(0);
                                }

                                free(pipe_args);
			        //goto nextprompt;
			    }
			   }
			  }
			  else { // "|&"
			    pid1 = fork();
                            if (pid1 == 0){
                             printf("pipe successful\n");
                             dup2(fds[1], STDOUT_FILENO);
                             dup2(fds[1], STDERR_FILENO);
			     close(fds[0]);
                             close(fds[1]);
                             execvp(execargs[0], execargs);
                             //execve(execargs[0], execargs, NULL);

                             exit(0);

                            } else {
                                pid2 = fork();

                                if(pid2 == 0){

                                dup2(fds[0], STDIN_FILENO);
				dup2(fds[0], STDERR_FILENO);
                                close(fds[1]);
                                close(fds[0]);
                                execvp(pipe_args[0], pipe_args);
                              //execve(pipe_args[0], pipe_args, NULL);

                                exit(0);
                                }else{
                                   close(fds[0]);
                                   close(fds[1]);
                                   waitpid(pid1, NULL, WNOHANG);
                                   waitpid(pid2, NULL, WNOHANG);
                                   sleep(1);

                                   for (int i = 0; i < MAXARGS; i++) {
                                      if(pipe_args[i] != NULL){
                                        free(pipe_args[i]);
                                      }
                                   }
				   exit(0);
                                   free(pipe_args);
                                } 
                               }
			     }
			                    /* parent */
                  	if (! background) { // foreground process
			if ((pid1 = waitpid(pid1, &status, 0)) < 0 || (pid2 = waitpid(pid2, &status, 0)) < 0)
                      		printf("waitpid error");
                 	}
                  	else {              // no waitpid if background
                    	while (1) {
		          pid_t terminated_pid1 = waitpid(pid1, &status, WNOHANG);
                          pid_t terminated_pid2 = waitpid(pid2, &status, WNOHANG);
                      	  if (terminated_pid1 == -1 || terminated_pid2 == -1) {
                        	perror("waitpid failed");
                        	exit(1);
                      	}
                      	if (terminated_pid1 == pid1 || terminated_pid2 == pid2) {
                        	/**if (WIFEXITED(status)) {
                          	printf("Child process with PID %d exited normally with status %d\n", pid, WEXITSTATUS(status));
                        } else {
                          printf("Child process with PID %d exited abnormally\n", pid);
                        }**/
                          break;
                     	}
                     //printf("Child process with PID %d is still running\n", pid);
                         sleep(1);
                       }
                         background = 0;
                       }
		       goto nextprompt;
                     }
			//}

			else execve(execargs[0], execargs, NULL);
			printf("couldn't execute: %s", buf);
			exit(127);
		  }

		  

		  /* parent */
		  if (! background) { // foreground process
		    if ((pid = waitpid(pid, &status, 0)) < 0)
	     	      printf("waitpid error");
                  }
		  else {              // no waitpid if background
		    while (1) {
            	      pid_t terminated_pid = waitpid(pid, &status, WNOHANG);
            	      if (terminated_pid == -1) {
                	perror("waitpid failed");
                	exit(1);
            	      }
            	      if (terminated_pid == pid) {
                        /**if (WIFEXITED(status)) {
                          printf("Child process with PID %d exited normally with status %d\n", pid, WEXITSTATUS(status));
                        } else {
                          printf("Child process with PID %d exited abnormally\n", pid);
                        }**/
                        break;
                     }
                     //printf("Child process with PID %d is still running\n", pid);
                     sleep(1);
                   }
		   background = 0;
		  }
/**
                  if (WIFEXITED(status)) S&R p. 239 
                    printf("child terminates with (%d)\n", WEXITSTATUS(status));
**/
                }

           nextprompt:
		fprintf(stdout, ">> ");
		fflush(stdout);
	}
	exit(0);
}
