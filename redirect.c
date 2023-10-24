#include "redirect.h"

void redirect(int oldfd) {
        if (dup(oldfd) == -1) {
            perror("dup2");
            exit(EXIT_FAILURE);
        }
        close(oldfd);
}

void handle_redirection(char **args, int arg_no) {
    int i, fd;

    printf("in redirect");

    for (i = 0; i < arg_no; i++) {
        if (strcmp(args[i], ">") == 0) {
            if (i+1 >= arg_no) {
                fprintf(stderr, "syntax error near unexpected token `newline'\n");
                return;
            }
            fd = open(args[i+1], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
            if (fd == -1) {
                perror(args[i+1]);
                return;
            }
            redirect(fd);
            args[i] = NULL;
            i++;
        }
        else if (strcmp(args[i], ">&") == 0) {
            if (i+1 >= arg_no) {
                fprintf(stderr, "syntax error near unexpected token `newline'\n");
                return;
            }
            fd = open(args[i+1], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
            if (fd == -1) {
                perror(args[i+1]);
                return;
            }
            redirect(fd);
            redirect(fd);
            args[i] = NULL;
            i++;
        }
        else if (strcmp(args[i], ">>") == 0) {
            if (i+1 >= arg_no) {
                fprintf(stderr, "syntax error near unexpected token `newline'\n");
                return;
            }
            fd = open(args[i+1], O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
            if (fd == -1) {
                perror(args[i+1]);
                return;
            }
            redirect(fd);
            args[i] = NULL;
            i++;
        }
        else if (strcmp(args[i], ">>&") == 0) {
            if (i+1 >= arg_no) {
                fprintf(stderr, "syntax error near unexpected token `newline'\n");
                return;
            }
            fd = open(args[i+1], O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
            if (fd == -1) {
                perror(args[i+1]);
                return;
            }
            redirect(fd);
            redirect(fd);
            args[i] = NULL;
            i++;
        }
        else if (strcmp(args[i], "<") == 0) {
            if (i+1 >= arg_no) {
                fprintf(stderr, "syntax error near unexpected token `newline'\n");
                return;
            }
            fd = open(args[i+1], O_RDONLY);
            if (fd == -1) {
                perror(args[i+1]);
                return;
            }
            redirect(fd);
            args[i] = NULL;
            i++;
        }
    }
}
