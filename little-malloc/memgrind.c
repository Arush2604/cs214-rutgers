#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "mymalloc.h"

void test_A() {
    
    //structs to track time
    struct timeval start;
    struct timeval end;

    int a_time = 0;
    //outer loop iterates 50 times
    for(int k = 0; k < 50;k++) {

        gettimeofday(&start, 0);

        //innter loop iterates 120 times
        for(int i = 0; i < 120; i++) {
            int* a = malloc(sizeof(char));
            free(a);

            //Clear memory and metadata 
            clearMemory();

        }

        gettimeofday(&end, 0);

        long seconds = (end.tv_sec - start.tv_sec);
        long micros = ((seconds * 1000000) + end.tv_usec) - (start.tv_usec);

        //increments time
        a_time = a_time + seconds + micros;

    }
    printf("Average time taken for test A: %d microseconds\n", a_time/50);

}

void test_B() {
    struct timeval start;
    struct timeval end;
    int b_time = 0;

    //Pointer array 
    int* mallocs[120];

   //loop iterates 50 times
   for(int k = 0; k < 50; k++) {
    gettimeofday(&start, 0);

    //filling up the array with pointers to 1 byte chunks
    for(int i = 0; i <120; i++) {
        mallocs[i] = malloc(sizeof(char));
    }

    //freeing up array
    for(int i = 0; i < 120; i++) {
        free(mallocs[i]);
    }

    //Clear memory and metadata 
    clearMemory();

    gettimeofday(&end, 0);

    long seconds = (end.tv_sec - start.tv_sec);
    long micros = ((seconds * 1000000) + end.tv_usec) - (start.tv_usec);

    //increments time taken
    b_time = b_time + seconds + micros;

   }
   printf("Average time taken for test B: %d microseconds\n", b_time/50);
}

void test_C() {

    struct timeval start;
    struct timeval end;
    int c_time = 0;

    //pointer array
    int* c_mallocs[120];

   //outer loop iterates 50 times
   for(int k = 0; k < 50; k++) {

    gettimeofday(&start, 0);

    //keeps track of how many times malloc is called
    int calls = 0;
    int x;
    
    //amount of chunks allocated
    int msize = 0;

    //random number generator initiliazer
    srand(time(0));


    while(calls < 120) {

        //calls malloc if array size is 0, generates randomly if otherwise
        if(msize == 0) {
            x = 0;
        }
        else {
            int rnd = rand();
            x =  rnd % 2;

        }

        //Malloc() if 0
        if(x == 0) {

            c_mallocs[msize+1] = malloc(sizeof(char));
            calls++;
            msize++;
        }

        //Free() if 1
        else {
            free(c_mallocs[msize]);
            msize--;
  
        }

    }

    //free remainder of array
    while (msize > 0) {
        free(c_mallocs[msize]);
        msize--;
    }

    //Clear memory and metadata 
    clearMemory();

    gettimeofday(&end, 0);

    long seconds = (end.tv_sec - start.tv_sec);
    long micros = ((seconds * 1000000) + end.tv_usec) - (start.tv_usec);

    c_time = c_time + seconds + micros;
    
   }

   printf("Average time taken for test C: %d microseconds\n", c_time/50);

}

void test_D() {
    struct timeval start;
    struct timeval end;

    int d_time = 0;

    //outer loop iterates 50 times
    for(int k = 0; k < 50;k++) {

        gettimeofday(&start, 0);

        //inner loop iterates 120 times
        for(int i = 0; i < 120; i++) {

            int* a = malloc(sizeof(int));
            int* b = malloc(sizeof(char));
            int* c = malloc(sizeof(long));

            free(a);
            free(b);
            free(c);

            //Clear memory and metadata 
            clearMemory();

        }

        gettimeofday(&end, 0);

        long seconds = (end.tv_sec - start.tv_sec);
        long micros = ((seconds * 1000000) + end.tv_usec) - (start.tv_usec);

        d_time = d_time + seconds + micros;

    }

    printf("Average time taken for test D: %d microseconds\n", d_time/50);
}

void test_E() {
    struct timeval start;
    struct timeval end;

    typedef struct block {
        int a;
        long b;
        char c;
        char d;
    } Block;

    int e_time = 0;

    //outer loop iterates 50 times
    for(int k = 0; k < 50;k++) {

        gettimeofday(&start, 0);

        //array to store pointer
        int* e_mallocs[120];

        //fill up array
        for(int i = 0; i < 120; i++) {
            e_mallocs[i] = malloc(sizeof(Block));

        }

        //free array
        for(int j = 0; j < 120; j++) {
            free(e_mallocs[j]);
        }

        //Clear memory and metadata 
        clearMemory();
        
        gettimeofday(&end, 0);

        long seconds = (end.tv_sec - start.tv_sec);
        long micros = ((seconds * 1000000) + end.tv_usec) - (start.tv_usec);

        e_time = e_time + seconds + micros;

    }

    printf("Average time taken for test E: %d microseconds\n", e_time/50);
}

int main(int argc, char const *argv[]) {
    //calls all tests
    
    test_A();
    test_B();
    test_C();
    test_D();
    test_E();

    return EXIT_SUCCESS;
}







