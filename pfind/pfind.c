/*******************************************************************************
 * Name        : pfind.c
 * Author      : Marjan Chowdhury
 * Description : Permission Find main method and implementation.
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <dirent.h>
#include <limits.h>
#include <unistd.h>
#include <stdbool.h>

int perms[] = {S_IRUSR, S_IWUSR, S_IXUSR,
               S_IRGRP, S_IWGRP, S_IXGRP,
               S_IROTH, S_IWOTH, S_IXOTH};

void display_usage(){
    printf("Usage: ./pfind -d <directory> -p <permission string> [-h]\n");
}

bool verify_perms(char *permissions){
    bool result = true;
    if (strlen(permissions) != 9){
        result = false;
    }
    for (int i = 0; i <= 8 && result; i++){
        if (i % 3 == 0){
            if (*(permissions + i) != 'r' && *(permissions + i) != '-'){
                result = false;
            }
        }else if (i % 3 == 1){
            if (*(permissions + i) != 'w' && *(permissions + i) != '-'){
                result = false;
            }
        }else{
            if (*(permissions + i) != 'x' && *(permissions + i) != '-'){
                result = false;
            }
        }
    }
    return result;
}


char *permission_string(struct stat *statbuf){
    char *str = (char *)malloc(11 * sizeof(char));
    if (str == NULL){
        exit(EXIT_FAILURE);
    }
    char *str_iterator = str;
    //*str_iterator++ = '-';
    int permission_valid;
    for (int i = 0; i < 9; i += 3){
        permission_valid = statbuf->st_mode & perms[i];
        if (permission_valid){
            *str_iterator++ = 'r';
        }else{
            *str_iterator++ = '-';
        }
        permission_valid = statbuf->st_mode & perms[i + 1];
        if (permission_valid){
            *str_iterator++ = 'w';
        }else{
            *str_iterator++ = '-';
        }
        permission_valid = statbuf->st_mode & perms[i + 2];
        if (permission_valid){
            *str_iterator++ = 'x';
        }else{
            *str_iterator++ = '-';
        }
    }
    *str_iterator = '\0';

    return str;
}

int navigate(char *directory, char* perms){
    char copy[PATH_MAX];
    struct dirent *de;
    DIR *dir;
    if ((dir = opendir(directory)) == NULL){
        fprintf(stderr,"Error: Cannot open directory '%s'. %s\n",directory,strerror(errno));
        return EXIT_FAILURE;
    }
    
    
    while ((de = readdir(dir)) != NULL){
        if (strcmp(de->d_name, ".") == 0 ||
            strcmp(de->d_name, "..") == 0){
            continue;
        }
        strcpy(copy, directory);
        strcat(copy, "/");
        strcat(copy, de->d_name);
        struct stat b;
        if (lstat(copy, &b) < 0){
            fprintf(stderr, "Error: Cannot stat '%s'. %s.\n", copy,
                    strerror(errno));
            continue;
        }
        char *file_perm = permission_string(&b);

        if (strcmp(file_perm,perms)==0){
            printf("%s\n",copy);
        }
        
        if (de->d_type == DT_DIR){
            navigate(copy, perms);
        }
        free(file_perm);
    }
    closedir(dir);
    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]){
    if (argc == 1){
        display_usage();
        return EXIT_FAILURE;
    }

    bool d_flag = false;
    bool p_flag = false;
    char directory[PATH_MAX];
    char permissions[10];
    int opt = -1;
    while ((opt = getopt(argc, argv, ":d:p:h")) != -1){
        switch (opt){
        case 'd':
            strcpy(directory, optarg);
            d_flag = true;
            break;
        case 'p':
            strcpy(permissions, optarg);
            p_flag = true;
            break;
        case 'h':
            display_usage();
            return EXIT_SUCCESS;
        case '?':
            fprintf(stderr, "Error: Unknown option '-%c' received.\n", optopt);
            return EXIT_FAILURE;
        }
    }

    if(!d_flag){
        fprintf(stderr, "Error: Required argument -d <directory> not found.\n");
        return EXIT_FAILURE;
    }

    if (!p_flag){
        fprintf(stderr, "Error: Required argument -p <permissions string> not found.\n");
        return EXIT_FAILURE;
    }

    struct stat statbuf;
    if (stat(directory, &statbuf) < 0){
        fprintf(stderr, "Error: Cannot stat '%s'. %s.\n", directory,
                strerror(errno));
        return EXIT_FAILURE;
    }
    if (!(S_ISDIR(statbuf.st_mode))){
        fprintf(stderr, "Error: '%s' is not a directory.\n", directory);
        return EXIT_FAILURE;
    }
    if (!verify_perms(permissions)){
        fprintf(stderr, "Error: Permission string '%s' is invalid\n", permissions);
        return EXIT_FAILURE;
    }
    char buf[PATH_MAX];
    char *path = realpath(directory,buf);
    if (path == NULL){
        fprintf(stderr, "Error: Cannot get full path of file '%s'. %s\n",
                directory, strerror(errno));
        return EXIT_FAILURE;
    }

    int result = navigate(path, permissions);
    return result;

}