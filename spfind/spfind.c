/*******************************************************************************
 * Name        : spfind.c
 * Author      : Marjan Chowdhury
 * Description : Sorted Permission Find Implementation
 ******************************************************************************/
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

bool starts_with(const char *str, const char *prefix){
    if (strlen(prefix) > strlen(str)){
        return false;
    }else{
        return !strncmp(str, prefix, strlen(prefix));
    }
}

int main(int argc, char *argv[]){
    int pfind_to_sort[2], sort_to_parent[2];
    
    if(pipe(pfind_to_sort) < 0){
        fprintf(stderr, "Error: pipe() failed. %s\n", strerror(errno));
        return EXIT_FAILURE;
    }
    if (pipe(sort_to_parent) < 0){
        fprintf(stderr, "Error: pipe() failed. %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    pid_t pid[2];
    if ((pid[0] = fork()) < 0){
        fprintf(stderr, "Error: fork() failed. %s.\n", strerror(errno));
        return EXIT_FAILURE;
    
    }
    else if ((pid[0] == 0)){
            close(pfind_to_sort[0]); close(sort_to_parent[0]); close(sort_to_parent[1]);
            
            if(dup2(pfind_to_sort[1], STDOUT_FILENO) < 0){
                fprintf(stderr, "Error: dup2() failed. %s\n", strerror(errno));
                return EXIT_FAILURE;
            }
            
            if(execv("pfind", argv) < 0){
                fprintf(stderr, "Error: pfind failed.\n");
                return EXIT_FAILURE;
            }
    
    }

    if ((pid[1] = fork()) < 0){
            fprintf(stderr, "Error: fork() failed. %s.\n", strerror(errno));
            return EXIT_FAILURE;
    
    }
    else if ((pid[1] == 0)){
            close(pfind_to_sort[1]); close(sort_to_parent[0]);
            
            if(dup2(pfind_to_sort[0], STDIN_FILENO) < 0){
                fprintf(stderr, "Error: dup2() failed. %s\n", strerror(errno));
                return EXIT_FAILURE;
            }
            
            if(dup2(sort_to_parent[1], STDOUT_FILENO) < 0){
                fprintf(stderr, "Error: dup2() failed. %s\n", strerror(errno));
                return EXIT_FAILURE;
            }
            
            if(execlp("sort", "sort", NULL) < 0){
                fprintf(stderr, "Error: sort failed.\n");
                return EXIT_FAILURE;
            }
    
    }

    close(pfind_to_sort[0]); close(pfind_to_sort[1]); close(sort_to_parent[1]);
    if(dup2(sort_to_parent[0], STDIN_FILENO) < 0){
        fprintf(stderr, "Error: dup2() failed. %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    char buf[4096];
    int bytes_read = 0;
    int count = 0;
    bool error = false;

    while((bytes_read = read(STDIN_FILENO,buf, sizeof(buf)))>0){
        for(int i = 0; i <= bytes_read; ++i){
            if(write(STDOUT_FILENO, buf+i, 1) < 0){
                fprintf(stderr, "Error: write() failed. %s\n", strerror(errno));
                return EXIT_FAILURE;
            }
            if(*(buf+i) == '\n'){
                ++count;
            }
            
        } 
    }
    if(bytes_read < 0){
        fprintf(stderr, "Error: read() failed. %s\n", strerror(errno));
        return EXIT_FAILURE;
    }
    if (starts_with(buf, "Usage:")){
        error = true;
    }

    int status;
    for(int i = 0; i< 2; ++i){
        pid_t wpid = waitpid(pid[i], &status, 0);
        if(WEXITSTATUS(status) == EXIT_FAILURE){
            error = true;
        }
        if (wpid == -1){
            if(i == 0){
                fprintf(stderr, "Error: waitpid() failed for pfind.\n");
            }else{
                fprintf(stderr, "Error: waitpid() failed for sort.\n");
            }
            return EXIT_FAILURE;
        }
        if (!WIFEXITED(status) || (WEXITSTATUS(status) == EXIT_FAILURE)){
            return EXIT_FAILURE;
        }
    }
    if(!error){
        printf("Total matches: %d\n",count);
    }

    return EXIT_SUCCESS;
}