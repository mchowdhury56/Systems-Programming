/*******************************************************************************
 * Name        : minishell.c
 * Author      : Marjan Chowdhury
 * Description : Minishell Implementation
 ******************************************************************************/

#include <errno.h>
#include <limits.h>
#include <pwd.h>
#include <setjmp.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define BRIGHTBLUE "\x1b[34;1m"
#define DEFAULT "\x1b[0m"

volatile sig_atomic_t signal_val = 0;
sigjmp_buf jmpbuf;

void catch_signal(int sig){
    if(signal_val == 0){
        write(STDOUT_FILENO, "\n", 1);
        signal_val = 1;
        siglongjmp(jmpbuf, 1);
    }
    
}

int main(){
    int retval = EXIT_SUCCESS;
    char buf[4096]; 
    bool exit = false;
    int argc;
    char **argv = NULL;
    

    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = catch_signal;
    action.sa_flags = SA_RESTART;
    if (sigaction(SIGINT, &action, NULL) == -1){
        fprintf(stderr, "Error: Cannot register signal handler. %s.\n", strerror(errno));
        retval = EXIT_FAILURE;
        exit = true;
        goto EXIT;
    }

    sigsetjmp(jmpbuf, 1);
    if(signal_val == 1){
        goto EXIT;
    }
    while (true){
        char path[PATH_MAX];
        if (getcwd(path, sizeof(path)) == NULL){
            fprintf(stderr, "Error: Cannot get current working directory. %s.\n", strerror(errno));
            retval = EXIT_FAILURE;
            goto EXIT;
        }
        if (buf[0] == '('){ // in getcwd It is possible we will mess up when implementing something like cd, and and the current directory will not be below the root directory of the currebt process. In this case, the returned path will be prefixed with "(unreachable)", so we must check to see if the first charactar is "(", and if it is, we should likely throw an error.
            fprintf(stderr, "Error: Path unreachable.\n"); // I am unsure if this is the right error message, this case was not listed explicitely in "Error Handling". Maybe it should be the same thing as the previous error. However, if this happens, errno is not set up.
            retval = EXIT_FAILURE;
            goto EXIT;
        }
        printf("[%s%s%s]$ ", BRIGHTBLUE, path, DEFAULT);
        fflush(stdout);
        

        argc = 0;
        argv = NULL;
        memset(buf, 0, 4096);
        ssize_t bytes_read = read(STDIN_FILENO, buf, sizeof(buf) - 1);
        if (bytes_read < 0){
            fprintf(stderr, "Error: read() failed. %s\n", strerror(errno));
            retval = EXIT_FAILURE;
            goto EXIT;
        }
        if (bytes_read == 0){
            write(STDOUT_FILENO, "\n", 1);
            retval = EXIT_FAILURE;
            exit = true;
            goto EXIT;
        }
        if (bytes_read > 0){
            buf[bytes_read - 1] = '\0';
        }
        char buf3[4096];
        strcpy(buf3, buf);
        
        char *s = strtok(buf, " ");
        if(s == NULL){
            goto EXIT;
        }
        if((argv = (char **)malloc(4096*sizeof(char *))) == NULL){
            fprintf(stderr, "Error: malloc() failed. %s.\n", strerror(errno));
            retval = EXIT_FAILURE;
            goto EXIT;
        }
        do{
            if ((argv[argc] = calloc(strlen(s), sizeof(char))) == NULL){
                fprintf(stderr, "Error: calloc() failed. %s.\n", strerror(errno));
                retval = EXIT_FAILURE;
                goto EXIT;
            
            }
            
            strcpy(argv[argc], s);
            ++argc;
        
        }while((s = strtok(NULL, " ")) != NULL);
        for(int i = argc; i < sizeof(argv); ++i){
            if(argv[i] != NULL){
                argv[i] = NULL;
            }
        }
        if (strncmp(argv[0], "cd", 3) == 0){
            char *dir;
            if (argc == 1 || (argc == 2 && (strncmp(argv[1], "~", 5) == 0))){ // we want cd ~sometext to error out, but cd ~ to work, because thats how the shell and borowski do it
                if ((dir = getpwuid(getuid())->pw_dir) == NULL){
                    fprintf(stderr, "Error: Cannot get passwd entry. %s.\n", strerror(errno));
                    retval = EXIT_FAILURE;
                    goto EXIT;
                }
            
            }else {
                char dir2[PATH_MAX];
                int dir_size = 0;
                bool quotes_seen = false; // if an odd number of quotes have been seen
                int bufIndex = 3; // 0 through 2 are "cd " UNLESS THERE ARE LEADING SPACES
                // we deal with leading spaces:
                int bufSpaceCount = 0;
                while (buf[bufSpaceCount] == ' '){
                    bufSpaceCount++;
                    bufIndex++;
                }
                // now we need to check for spaces between cd and the path
                while (buf[bufIndex] == ' '){
                    bufIndex++;
                }
                
                //now we build up dir2 to hold the path
                while (buf3[bufIndex] != '\0') {
                    if (buf3[bufIndex] == '"') {
                        quotes_seen = !quotes_seen;
                    } else if (buf3[bufIndex] == ' ') {
                        if (!quotes_seen) {
                            // at this point we've seen a space after an even number of quotes, so we either have too many args, or we've seen a trailing space, lets find out which
                            while (buf3[++bufIndex] == ' ');
                            if (buf3[bufIndex] == '\0'){
                                break;
                            } else {
                                fprintf(stderr, "Error: Too many arguments to cd.\n");
                                retval = EXIT_FAILURE;
                                goto EXIT;
                            }
                        } else {
                            dir2[dir_size++] = ' ';
                        }
                    } else {
                        dir2[dir_size++] = buf3[bufIndex];
                    }
                    bufIndex++;
                }
                
                
                if (quotes_seen){
                    fprintf(stderr, "Error: Malformed command.\n");
                    retval = EXIT_FAILURE;
                    goto EXIT;
                }
                strncpy(dir2+dir_size, "\0", 1);
                dir_size++;
                
                if(!strncmp(dir2,"~",1) && strlen(dir2) >= 1){
                    if ((dir = getpwuid(getuid())->pw_dir) == NULL){
                        fprintf(stderr, "Error: Cannot get passwd entry. %s.\n", strerror(errno));
                        retval = EXIT_FAILURE;
                        goto EXIT;
                    }
                    memmove(dir2,dir2+1,strlen(dir2));
                    strcat(dir,dir2);
                
                }else{
                
                    dir = dir2;
                }
                //check to see if input was cd "" or cd """" or cd """""" and so on
                if (*dir == '\0') {
                    if ((dir = getpwuid(getuid())->pw_dir) == NULL){
                        fprintf(stderr, "Error: Cannot get passwd entry. %s.\n", strerror(errno));
                        retval = EXIT_FAILURE;
                        goto EXIT;
                    }
                }
            }
            if (chdir(dir) == -1){
                fprintf(stderr, "Error: Cannot change directory to '%s'. %s.\n", dir, strerror(errno));
                retval = EXIT_FAILURE;
                goto EXIT;
                
            }
            
            goto EXIT;
        
        }else if (strncmp(argv[0], "exit", 5) == 0){
            
            retval = EXIT_SUCCESS;
            exit = true;
            goto EXIT;
        
        }else{
            
            pid_t pid = fork();
            if (pid < 0){
                fprintf(stderr, "Error: fork() failed. %s.\n", strerror(errno));
                    retval = EXIT_FAILURE;
                    goto EXIT;
            
            }else if (pid == 0){
                if (execvp(argv[0], argv) < 0){
                    fprintf(stderr, "Error: exec() failed. %s.\n", strerror(errno));
                    _exit(0);
                    goto EXIT;
                }
            
            }else{
                signal_val = 1;
                int status;
                pid_t wpid = waitpid(pid, &status, 0);
                if (wpid == -1){
                    fprintf(stderr, "Error: wait() failed. %s.\n", strerror(errno));
                    retval = EXIT_FAILURE;
                    goto EXIT;
                }
                if (!WIFEXITED(status) || (WEXITSTATUS(status) == EXIT_FAILURE)){
                    retval = EXIT_FAILURE;
                    goto EXIT;
                }
            
            }
        
        }
            
        EXIT:
            signal_val = 0;
            if (argv != NULL){
                for (int i = 0; i <= sizeof(argv); ++i){ 
                    free(argv[i]);
                }
                free(argv);

            }
            if(exit){
                break;
            }
    
    }

    return retval;

}

