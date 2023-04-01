#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "mymalloc.h"

void invalidArguments() {
    printf("Your arguments are invalid (or lack thereof). Please provide a test case number, from 1-12, or \"all\" to test all test cases.\n");
}

// malloc tests

void oneAllocateOne() {
    malloc(sizeof(int));
    assert(getMemChunkLen() == 2);

    printf("test 1 passed\n");
}

void twoAllocateTwo() {
    malloc(sizeof(int));
    malloc(sizeof(char));
    assert(getMemChunkLen() == 3);

    printf("test 2 passed\n");
}

void threeUseAllMemory() {
    char* a = malloc(sizeof(char));

    while(a != NULL) {
        a = malloc(sizeof(char));
    }

    printf("test 3 passed (dependent on stdout)\n");
}

void fourAllocateOneAndWrite() {
    int* a = malloc(sizeof(int));
    assert(getMemChunkLen() == 2);

    *a = 4;
    assert(*a == 4);

    printf("test 4 passed\n");
}

void fiveAllocateTwoAndWrite() {
    int* a = malloc(sizeof(int));
    assert(getMemChunkLen() == 2);

    *a = 1;

    int* b = malloc(sizeof(long));
    assert(getMemChunkLen() == 3);

    *b = 2;

    assert(*a == 1);
    assert(*b == 2);

    printf("test 5 passed\n");
}

// free tests

void sixFreeOutOfBounds() {
    char* a = malloc(sizeof(char));    
    free(a - 1);

    printf("test 6 passed (dependent on stdout)\n");
}

void sevenFreeRandomPointer() {
    char* a = malloc(sizeof(char));
    free(a + 5);

    printf("test 7 passed (dependent on stdout)\n");
}

void eightFreePointerAfterChunk() {
    int* a = malloc(sizeof(int));
    // the difference between test 7 and 8 is that in this test, a + 2 still lies in the same chunk.
    free(a + 2);

    printf("test 8 passed (dependent on stdout)\n");
}

void nineFreeSamePointerTwice() {
    int* a = malloc(sizeof(int));
    free(a);

    assert(getMemChunkLen() == 1);

    free(a);

    printf("test 9 passed (dependent on stdout)\n");
}

void tenFreeOnePointerAndCoalesce() {
    int* a = malloc(sizeof(int));
    assert(getMemChunkLen() == 2);
    free(a);

    assert(getMemChunkLen() == 1);

    printf("test 10 passed\n");
}

void elevenFreeOnePointerBetweenAndCoalesce() {
    int* a = malloc(sizeof(int));
    assert(getMemChunkLen() == 2);

    int* b = malloc(sizeof(int));
    assert(getMemChunkLen() == 3);

    int* c = malloc(sizeof(int));
    assert(getMemChunkLen() == 4);

    free(a);
    free(c);

    assert(getMemChunkLen() == 3);

    free(b);

    assert(getMemChunkLen() == 1);

    printf("test 11 passed\n");
}

void twelveFreeNullPointer() {
    free(NULL);

    printf("test 12 passed\n");
}

int main(int argc, char* argv[]) {
    if(argc == 1 || argc > 2) {
        invalidArguments();
    } else {
        if(!strcmp(argv[1], "1")) {
            oneAllocateOne();
        } else if(!strcmp(argv[1], "2")) {
            twoAllocateTwo();
        } else if(!strcmp(argv[1], "3")) {
            threeUseAllMemory();
        } else if(!strcmp(argv[1], "4")) {
            fourAllocateOneAndWrite();
        } else if(!strcmp(argv[1], "5")) {
            fiveAllocateTwoAndWrite();
        } else if(!strcmp(argv[1], "6")) {
            sixFreeOutOfBounds();
        } else if(!strcmp(argv[1], "7")) {
            sevenFreeRandomPointer();
        } else if(!strcmp(argv[1], "8")) {
            eightFreePointerAfterChunk();
        } else if(!strcmp(argv[1], "9")) {
            nineFreeSamePointerTwice();
        } else if(!strcmp(argv[1], "10")) {
            tenFreeOnePointerAndCoalesce();
        } else if(!strcmp(argv[1], "11")) {
            elevenFreeOnePointerBetweenAndCoalesce();
        } else if(!strcmp(argv[1], "12")) {
            twelveFreeNullPointer();
        } else if(!strcmp(argv[1], "all")) {
            oneAllocateOne();
            clearMemory();
            twoAllocateTwo();
            clearMemory();
            threeUseAllMemory();
            clearMemory();
            fourAllocateOneAndWrite();
            clearMemory();
            fiveAllocateTwoAndWrite();
            clearMemory();
            sixFreeOutOfBounds();
            clearMemory();
            sevenFreeRandomPointer();
            clearMemory();
            eightFreePointerAfterChunk();
            clearMemory();
            nineFreeSamePointerTwice();
            clearMemory();
            tenFreeOnePointerAndCoalesce();
            clearMemory();
            elevenFreeOnePointerBetweenAndCoalesce();
            clearMemory();
            twelveFreeNullPointer();
        } else {
            invalidArguments();
        }
    }
}






