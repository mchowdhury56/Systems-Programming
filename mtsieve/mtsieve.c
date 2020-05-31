/*******************************************************************************
 * Name        : mtsieve.c
 * Author      : Marjan Chowdhury
 * Description : Multithreaded Primes Sieve Implementation
 ******************************************************************************/

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <time.h>
#include <unistd.h>

int total_count = 0;
pthread_mutex_t lock;

typedef struct arg_struct {
    int start;
    int end;
} thread_args;

void *segmented_sieve(void *ptr){
    thread_args *args = (thread_args *)ptr;
    const int start = args->start, end = args->end;
    int range = end - start + 1;
    int rootb = (int) ceil(sqrt(end));

    int lengthOfSharedArray; // we will be using one bool array for both sieves. 
    if (rootb+1 > range){ // we need it to be large enough to fit range, because we are going to reuse this chunk of "heap" instead of mallocing twice
        lengthOfSharedArray = rootb+1;
    } else { 
        lengthOfSharedArray = range;
    }
    
    bool *sieve;
    if((sieve = malloc(lengthOfSharedArray*sizeof(bool))) == NULL){
        fprintf(stderr, "Error: malloc() failed. %s.\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    int low_primes[rootb], lowIndex = 0; 

    for(int i = 0; i < rootb+1; i++){
        sieve[i] = true;
    }  
    
    //sieve lowprimes
    for(int i = 2; i <= rootb; i++){
        if(sieve[i]){
            low_primes[lowIndex++] = i;
            for(int j = i*2; j <= rootb; j += i){
                sieve[j] = false;
            }
        }
    }

    bool *high_primes = &sieve[0]; 
    for (int i = 0; i < range; i++){
        high_primes[i] = true;
    }
    for (int i = range; i < lengthOfSharedArray; i++){
        high_primes[i] = false;
    }
    
    //preform the segmented sieve
    for(int p = 0; p < lowIndex; p++){
        
        int i = (int)ceil((double)start/low_primes[p]) * low_primes[p] - start;
        if (start <= low_primes[p]){
            i += low_primes[p];
        }
        for(i = i; i < range; i += low_primes[p]){
            high_primes[i] = false;
        }
    }

    for(int j = 0; j < range; j++){
        if(high_primes[j]){
            int num = j+start;
            int threeCount = 0;
            while(num != 0) {
                if(num % 10 == 3){ 
                    if(++threeCount == 2){
                        int retval;
                        if ((retval = pthread_mutex_lock(&lock)) != 0){
                            fprintf(stderr, "Warning: Cannot lock mutex. %s.\n", strerror(retval));
                        }
                        total_count++;
                        if ((retval = pthread_mutex_unlock(&lock)) != 0){
                            fprintf(stderr, "Warning: Cannot unlock mutex. %s.\n", strerror(retval));
                        }
                        break;
                    }
                }
                num /= 10;
            }
        }
    }
    free(high_primes);
    return NULL;
}


int input_check(char *optarg, char flag){ 
    //this loop 1) asserts that optarg is an integer, 2) constructs that integer, and 3) checks for integer overflow
    int val = 0, thisDigit;
    
    for (int i = 0; optarg[i] != '\0'; i++){ 
        //assert char is digit
        if (!isdigit(optarg[i])){
            fprintf(stderr, "Error: Invalid input '%s' received for parameter '-%c'.\n", optarg, flag);
            exit(EXIT_FAILURE);
        }

        //add digit to val after multiplying val by 10 (because adding a digit increases significance of existing digits)
        val = val * 10;
        thisDigit = optarg[i] - '0';
        val += thisDigit;

        //check for overflow
        if (val < 0){
            fprintf(stderr, "Error: Integer overflow for parameter '-%c'.\n", flag);
            exit(EXIT_FAILURE);
        }
    }
    return val;
}


void error_check(int type, bool flag, int value){
    if(type == 0){
        if(!flag){
            fprintf(stderr, "Error: Required argument <starting value> is missing.\n");
            exit(EXIT_FAILURE);
        }
        if(value < 2){
            fprintf(stderr, "Error: Starting value must be >= 2.\n");
            exit(EXIT_FAILURE);
        }
    }else if(type == 1){
        if(!flag){
            fprintf(stderr, "Error: Required argument <ending value> is missing.\n");
            exit(EXIT_FAILURE);
        }
        if(value < 2){
            fprintf(stderr, "Error: Ending value must be >= 2.\n");
            exit(EXIT_FAILURE);
        }
    }else{
        if(!flag){
            fprintf(stderr, "Error: Required argument <num threads> is missing.\n");
            exit(EXIT_FAILURE);
        }
        if(value < 1){
            fprintf(stderr, "Error: Number of threads cannot be less than 1.\n");
            exit(EXIT_FAILURE);
        }
        int num_processors = get_nprocs();
        if(value > 2*num_processors){
            fprintf(stderr, "Error: Number of threads cannot exceed twice the number of processors(%d).\n",num_processors);
            exit(EXIT_FAILURE);
        }
    }

}

int main(int argc, char *argv[]){
    if(argc == 1){
        fprintf(stderr, "Usage: ./mtsieve -s <starting value> -e <ending value> -t <num threads>\n");
        return EXIT_FAILURE;
    }
    
    int start = -1, end = -1, num_threads = -1;
    bool e_flag = false, s_flag = false, t_flag  = false;
    int opt = -1;
    opterr = 0;
    while ((opt = getopt(argc, argv, "e:s:t:")) != -1){ 
        switch (opt){
        case 'e':
            end = input_check(optarg,'e');
            e_flag =!e_flag;
            break;
        case 's':
            start = input_check(optarg,'s');
            s_flag = !s_flag;
            break;
        case 't':
            num_threads = input_check(optarg,'t');
            t_flag = !t_flag;
            break;
        case '?':
            if (optopt == 'e' || optopt == 's' || optopt == 't') {
                fprintf(stderr, "Error: Option -%c requires an argument.\n", optopt);
            } else if (isprint(optopt)) {
                fprintf(stderr, "Error: Unknown option '-%c'.\n", optopt);
            } else {
                fprintf(stderr, "Error: Unknown option character '\\x%x'.\n",optopt);
            }
            return EXIT_FAILURE;
        }
    }
    // optind is set up and used by getopt. it points to the next command line argument, or element, to be processed by getopt
    // after all the options are parsed, optind should equal argc, unless there were more arguments supplied that werent options
    if (optind < argc){
        fprintf(stderr, "Error: Non-option argument '%s' supplied.\n", argv[optind]);
        return EXIT_FAILURE;
    }
    error_check(0,s_flag,start);
    error_check(1,e_flag,end);
    if(end < start){
        fprintf(stderr, "Error: Ending value must be >= starting value.\n");
        return EXIT_FAILURE;
    }
    error_check(2,t_flag,num_threads);

    //create mutex
    int retval;
    if ((retval = pthread_mutex_init(&lock, NULL)) != 0) {
        fprintf(stderr, "Error: Cannot create mutex. %s.\n", strerror(retval));
        return EXIT_FAILURE;
    }

    pthread_t threads[num_threads];
    thread_args targs[num_threads];

    // segment the range
    int range = end - start + 1, segsize;
    if (num_threads > range){
        num_threads = range;
        segsize = 1;
    }else {
        segsize = range / num_threads;
    }
    int left_over = range - (segsize * num_threads); 
    /* Each process will have at least segsize numbers assigned to it, 
    the leftover will be distrivuted among all the threads, with later 
    segments containing less values. */

    //create the threads
    int thisStart = start;
    printf("Finding all prime numbers between %d and %d.\n",start,end);
    if (num_threads == 1){
        printf("%d segment:\n", num_threads);
    } else {
        printf("%d segments:\n", num_threads);
    }
    for(int i = 0; i<num_threads; ++i){
        targs[i].start = thisStart; 
        targs[i].end = thisStart + segsize - 1;
        thisStart += segsize;
        if (left_over > 0){
            targs[i].end += 1;
            left_over--;
            thisStart += 1;
        }
        printf("   [%d, %d]\n", targs[i].start, targs[i].end);
    }
    
    for(int i = 0; i<num_threads; ++i){
        if ((retval = pthread_create(&threads[i], NULL, segmented_sieve, (void *)&targs[i])) != 0){
            fprintf(stderr, "Error: Cannot create thread %d. %s.\n", i + 1, strerror(retval));
            return EXIT_FAILURE;
        }
    }
    //join the threads
    for (int i = 0; i < num_threads; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            fprintf(stderr, "Warning: Thread %d did not join properly.\n", i + 1);
        }
    }
    //destroy mutex
    if ((retval = pthread_mutex_destroy(&lock)) != 0) {
        fprintf(stderr, "Error: Cannot destroy mutex. %s.\n", strerror(retval));
        return EXIT_FAILURE;
    }

    printf("Total primes between %d and %d with two or more '3' digits: %d\n",start,end,total_count);
    return EXIT_SUCCESS;
}