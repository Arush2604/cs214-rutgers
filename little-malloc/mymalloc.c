#include <stdio.h>
#include <stdlib.h>

#define CHUNK_SIZE_BYTE_SIZE 4
#define FREE_BYTE_SIZE 1

#define MEM_BYTE_SIZE 4096
#define METADATA_SIZE (FREE_BYTE_SIZE + CHUNK_SIZE_BYTE_SIZE)
#define printError(msg) printf("usage error in %s, at line %d: %s\n", file, line, msg);

static char MEMORY[MEM_BYTE_SIZE];

// checks if memory array is initialized
int is_initialized() {
    return *((unsigned int*) (MEMORY + 1)) != 0;
}

// extracts the metadata info from an address, setting isOcc and chunkSize appropriately
void getMetadataInfo(char* current_metadata_addr, char** isOcc, int** chunkSize) {
    *isOcc = current_metadata_addr;
    *chunkSize = (int*) (current_metadata_addr + 1);
}

// initializes empty memory array
void initialize() {
    int* size = (int*) (MEMORY + 1);
    *size = MEM_BYTE_SIZE - METADATA_SIZE;
}

// prints out each chunk's metadata info
void printMemChunks() {
    if(is_initialized()) {
        char* current_metadata_addr = MEMORY;
        while(current_metadata_addr + METADATA_SIZE < MEMORY + MEM_BYTE_SIZE) {
            char* isOcc;
            int* chunkSize;
            getMetadataInfo(current_metadata_addr, &isOcc, &chunkSize);
        
            printf("|%d, %d|->", *isOcc, *chunkSize);

            // go on to the next memory chunk
            current_metadata_addr = current_metadata_addr + METADATA_SIZE + *chunkSize;
        }

        printf("END \n");
    } else {
        printf("Uninitialized memory\n");
    }
}

// returns the number of memory chunks
int getMemChunkLen() {
    if(is_initialized()) {
        int count = 0;
        char* current_metadata_addr = MEMORY;

        // traverse memory chunks
        while(current_metadata_addr + METADATA_SIZE < MEMORY + MEM_BYTE_SIZE) {
            char* isOcc;
            int* chunkSize;
            getMetadataInfo(current_metadata_addr, &isOcc, &chunkSize);
        
            count++;

            // go on to the next memory chunk
            current_metadata_addr = current_metadata_addr + METADATA_SIZE + *chunkSize;
        }

        return count;
    } else {
        return 0;
    }
}

// effectively clears the memory array
void clearMemory() {
    *MEMORY = 0;
    *((unsigned int*) (MEMORY + 1)) = 0;
}

// coalesces all freed chunks together
void coal() {
    char* current_metadata_addr = MEMORY;
    char* next_metadata_addr = current_metadata_addr + METADATA_SIZE + *((int*) (current_metadata_addr + 1));

    // traverse memory chunks
    while(next_metadata_addr + METADATA_SIZE  <= MEMORY + MEM_BYTE_SIZE) {

        // if two adjacent memory chunks are unoccupied, merge them into one
        if(*current_metadata_addr == 0 && *next_metadata_addr == 0) {
            *((int*) (current_metadata_addr + 1)) = *((int*) (current_metadata_addr + 1)) + METADATA_SIZE + *((int*) (next_metadata_addr + 1));
            next_metadata_addr = current_metadata_addr + METADATA_SIZE + *((int*) (current_metadata_addr + 1));            
        
        // else go to the next memory chunk pair
        } else {
            current_metadata_addr = next_metadata_addr;
            next_metadata_addr = next_metadata_addr + METADATA_SIZE + *((int*) (next_metadata_addr + 1));
        }
    }
}

void *mymalloc(size_t size, char *file, int line) {
    if(!is_initialized()) {
        initialize();
    }

    int validChunkSize = size + METADATA_SIZE + 1;

    char* current_metadata_addr = MEMORY;
    
    // traverse memory chunks
    while(current_metadata_addr + METADATA_SIZE < MEMORY + MEM_BYTE_SIZE) {
        char* isOcc;
        int* chunkSize;
        getMetadataInfo(current_metadata_addr, &isOcc, &chunkSize);

        // if memory chunk is unoccupied and has exactly the amount of space we need
        if(*isOcc == 0 && *chunkSize == size) {
            *isOcc = 1;
            return isOcc + METADATA_SIZE;

        // if memory chunk is unoccupied and has more space than we need
        } else if(*isOcc == 0 && *chunkSize >= validChunkSize) {
            char* newChunkMetadataAddr = current_metadata_addr + METADATA_SIZE + size;

            char* isNewOcc;
            int* newChunkSize;
            getMetadataInfo(newChunkMetadataAddr, &isNewOcc, &newChunkSize);
            
            // split the unoccupied memory chunk into two, occupied and unoccupied
            *isNewOcc = 0;
            *newChunkSize = *chunkSize - size - METADATA_SIZE;

            *isOcc = 1;
            *chunkSize = size;

            return (current_metadata_addr + METADATA_SIZE); 
        }

        // go on to the next memory chunk
        current_metadata_addr = current_metadata_addr + METADATA_SIZE + *chunkSize;
    }

    printError("not enough space to allocate memory")
    return NULL;
}

void myfree(void *ptr, char *file, int line) {
    if (ptr == NULL) {
        return;
    }

    char* current_metadata_addr = MEMORY;

    // traverse memory chunks
    while(current_metadata_addr <= (char*) ptr) {

        // if the current memory chunk's address is the same as the pointer passed
        if((current_metadata_addr + METADATA_SIZE) == ptr) {

            // if chunk is already freed, error, else free this chunk
            if(*current_metadata_addr == 0) {
                printError("cannot free already freed chunk");
            } else {
                *current_metadata_addr = 0; 
                coal();
            }

            return; 
        } else {
            // go on to the next memory chunk
            current_metadata_addr = current_metadata_addr + METADATA_SIZE + *((int*) (current_metadata_addr + 1));
        }

    }

    printError("this pointer does not point to allocated memory");
}


