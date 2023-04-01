lib: mymalloc.c
	gcc -Wall -Wmaybe-uninitialized -fsanitize=address -o mymalloc.o -c mymalloc.c

tests: tests.c
	gcc -Wall -fsanitize=address -o mymalloc.o -c mymalloc.c
	gcc -Wall -fsanitize=address tests.c mymalloc.o -o tests.o

memgrind: memgrind.c
	gcc -Wall -fsanitize=address -o mymalloc.o -c mymalloc.c
	gcc -Wall -fsanitize=address memgrind.c mymalloc.o -o memgrind.o
