# little-malloc

Group Members:
- Smail Barkouch (netid: sb2108)
- Arush Verma (netid: aav87)

# Design

We are going to make some baseline rules for our memory arrangement.
1. Every chunk of memory will have metadata. The metadata will have a size of five bytes, where the first byte indicates whether the chunk is free or not, and the next four bytes will be the size of the chunk.
2. Our memory will initialize as one chunk, the first five indexes of our memory array will be the first chunk's metadata. There will always be at least one chunk, and the first five bytes will always be the first chunks metadata.
3. When a free is called on a valid pointer, we will coalesce adjacent chunks. This will be done by checking the chunk to the left, and the chunk right, and coalescing whatever is free.

To see if our memory is initialized, we simply need to check the first five indexes of the memory array. If the first five indexes are 0, then the memory is uninitialized. This is because a chunk must always have a size greater than 0, or it is not a chunk, in our implementation.

# Test Plan

## Correct Properties of `malloc` and `free`

For `malloc` to be considered correct, we need to make sure that it has the following properties:
- gives us a pointer to free memory for the amount of bytes we specified
- bytes written to at occupied memory are preserved until freed
- never invalidates pointer

For `free` to be considered correct, we need to make sure that it has the following properties:
- detects whether an address is from malloc
    - to verify an address is from malloc, we will start at the beginning of our memory array. From the first chunk, we can navigate to every possible chunk using the metadata information. Since we know the size of each chunk, we can move between them. If we do not encounter the address, then we know that the address is not from malloc.
- makes sure an address is at the start of a writable chunk
- cannot free the same pointer twice
- will coalesce freed chunks to meet requested amount of bytes
- rejects null pointers

## Testing for Properties

To verify the properties we described previously, we will have tests and benchmarks.

### `malloc` Tests

Property 1: gives us pointer to free memory for the amount of bytes we specified
1. allocate one data type, verify it is in the memory array
2. allocate two data types, verify they are both in the memory array
3. allocate as many data types as it takes to occupy all the memory. `malloc` should return a null pointer when there is no memory left to allocate.

Property 2: bytes written at occupied memory are preserved until freed
1. allocate one data type, write to the pointer. verify that the data exists and the memory is occupied.
2. allocate two data types, write to both of them. verify that the data exists and memory is occupied.

Property 3: never invalidates pointer (no tests needed, tested retroactively by the previous properties)

### `free` Tests

Property 1: knows an address is from `malloc`
1. pass a pointer that is exists outside of the memory array (before first index of memory array)
2. pass a pointer that isn't allocated by malloc 

Property 2: makes sure an address is at the start of a chunk
1. free a pointer that exists after the start of the chunk

Property 3: cannot free the same pointer twice
1. free a valid pointer, and then attempt to free it again

Property 4: will coalesce freed chunks to meet requested amount of bytes
1. free a pointer that has an available chunk next to it
2. free a pointer that has available chunks on both sides

Propery 5: rejects null pointers
1. pass a null pointer

# File info

## `mymalloc.c` & `mymalloc.h`

The library files.

The errors for `malloc`:
- not enough bytes to fullfill request

The errors for `free`
- pointer is invalid (for the multitude of reasons previously described)
- already free chunk is attempted to be freed again

## `tests.c` - Correctness Testing

This file has the various test cases for our `mymalloc` library. The tests are listed from 1-12.

**Compile**: `make tests`

**Run**: `./tests.o <test # | all>`

## `memgrind.c` - Performance Testing

This file tests the speed of our `mymalloc` library.

**Compile**: `make memgrind`

**Run**: `./memgrind.o`

Tests in memgrind:
1) **"test_A"**: allocates a 1-byte chunk and then free's the chunk. It repeats this 120 times in the inner loop, and 50 times for the outer loop.

2) **"test_B"**: allocates 120 1-byte chunks, stores the pointers returned by malloc in an array and then frees the array. It repeats this 50 times.

3) **"test_C"**: randomly chooses an integer between 0 and 1. In the special case that the array is empty, 0 will be chosen. 

- *If 0 is chosen*, it will malloc a 1 byte chunk and store the pointer in an array. 
- *If a 1 is chosen*, the most recently allocated pointer will be freed.  

    Once malloc() is called 120 times, the rest of the pointers in the array (if any) will be freed.
    It will repeat this process 50 times.

4) **"test_D"**: allocates a chunk of 4, 1, and 8 bytes respectively (int, char, long). It then immediately frees the allocated chunks. It repeats this 120 times in the inner loop, and 50 times in the outer loop. 

5) **"test_E"**: creates a struct containing an int, a long and 2 characters. It then allocates a chunk the size of the struct 120 times and stores the pointers in an array. It then frees the pointers in the array. It repeats this 50 times. 

`memgrind` tracks the time taken after each of the 50 iterations, and averages them.

# Reporting Errors

Our error message will be as follows:
```
usage error in [file name], at line [line number]: {error information}
```
where brackets will be replaced by their respective information.
