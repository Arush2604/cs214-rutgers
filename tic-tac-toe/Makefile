build: ttt.c ttts.c tttx.c
	gcc -Wall -fsanitize=address -pthread ttt.c -o ttt.o
	gcc -Wall -fsanitize=address -pthread ttts.c -o ttts.o
	gcc -Wall -fsanitize=address -pthread tttx.c -o tttx.o

clean: ttt.o ttts.o tttx.o
	rm ttt.o ttts.o tttx.o
