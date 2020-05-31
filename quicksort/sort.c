/*******************************************************************************
 * Name        : sort.c
 * Author      : Marjan Chowdhury
 * Description : Uses quicksort to sort a file of either ints, doubles, or
 *               strings.
 ******************************************************************************/
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "quicksort.h"

#define MAX_STRLEN     64 // Not including '\0'
#define MAX_ELEMENTS 1024

typedef enum {
    STRING,
    INT,
    DOUBLE
} elem_t;

/**
 * Basic structure of sort.c:
 *
 * Parses args with getopt.
 * Opens input file for reading.
 * Allocates space in a char** for at least MAX_ELEMENTS strings to be stored,
 * where MAX_ELEMENTS is 1024.
 * Reads in the file
 * - For each line, allocates space in each index of the char** to store the
 *   line.
 * Closes the file, after reading in all the lines.
 * Calls quicksort based on type (int, double, string) supplied on the command
 * line.
 * Frees all data.
 * Ensures there are no memory leaks with valgrind. 
 */

void display_usage(){
    printf("Usage: ./sort [-i|-d] [filename]\n"
        "   -i: Specifies the file contains ints.\n"
        "   -d: Specifies the file contains doubles.\n"
        "   filename: The file to sort.\n"
        "   No flags defaults to sorting strings.\n");
}

int main(int argc, char **argv) {
    elem_t t = STRING;
    int opt = -1;
    while ((opt = getopt(argc, argv, ":i:d")) != -1){
        switch (opt){
        case 'i':
            t = INT;
            break;
        case 'd':
            t = DOUBLE;
            break;
        case '?':
            printf("Error: Unknown option '-%c' received.\n", optopt);
            display_usage();
            return EXIT_FAILURE;
        }
    }

    if (!((t == STRING && argc == 2)||(argc > 2))){
        display_usage();
        return EXIT_FAILURE;
    }

    char *filename;
    if (t == STRING){
        filename = argv[1];
    }else{
        filename = argv[2];
    }
    
    
    FILE *fp = fopen(filename, "r");
    if (fp == NULL){
        printf("Error: Cannot open '%s'. %s.\n", filename, strerror(errno));
        return EXIT_FAILURE;
    }
    
    char elem[MAX_STRLEN + 2];
    char **array = (void *)malloc(MAX_ELEMENTS+1 * sizeof(char *));
    int arrlength = 0;
    while (fgets(elem, MAX_STRLEN+2, fp) != NULL){
        char *element = (char *)malloc(MAX_STRLEN+2 * sizeof(char*));
        strcpy(element, elem);
        element[strlen(element) - 1] = '\0';
        array[arrlength++] = (char *)element;
    }
    fclose(fp);

    if(t == INT){
        int numarr[arrlength];
        int* intarr = (int *) numarr;
        for (size_t i = 0; array[i] != NULL; ++i){
            intarr[i] = atoi(array[i]);
        }
        quicksort(intarr, arrlength, sizeof(int), int_cmp);
        for (size_t i = 0; i < arrlength; ++i){
            printf("%d\n", intarr[i]);
        }
    }else if(t == DOUBLE){
        double dubarr[arrlength];
        double *dblarr = (double *)dubarr;
        for (size_t i = 0; i < arrlength; ++i){
            dblarr[i] = atof(array[i]);
        }
        quicksort(dblarr,arrlength, sizeof(double), dbl_cmp);
        for (size_t i = 0; array[i] != NULL; ++i){
            if (dblarr[i] == '\0'){
                break;
            }
            printf("%lf\n", dblarr[i]);
        }
    }else{
        quicksort(array,arrlength, sizeof(char *), str_cmp);
        for (size_t i = 0; i < arrlength; ++i){
            if(*array[i]=='\0'){
                break;
            }
            printf("%s\n", array[i]);
        }
    }

    for (size_t i = 0; i < arrlength; ++i) {
        free(array[i]);
    }
    free(array);
    
    return EXIT_SUCCESS;
}
