all:matrix_multiply

CC = g++
INCLUDE = .

CFLAGS = -Wall -g -fno-rtti -ftree-vectorize -march=native -DDEBUG -std=gnu++0x -O2 -D__THREADED_STRASSEN_FUNCTION__
LDFLAGS = -lboost_thread

matrix_multiply: main.o matrix.o Makefile
	$(CC) -I$(INCLUDE) $(CFLAGS) $(LDFLAGS) -o matrix_multiply main.o matrix.o

main.o: main.cpp matrix.h matrix.cpp Makefile
	$(CC) -I$(INCLUDE) $(CFLAGS) $(LDFLAGS) -c main.cpp

matrix.o: matrix.cpp matrix.h Makefile
	$(CC) -I$(INCLUDE) $(CFLAGS) $(LDFLAGS) -c matrix.cpp

clean:
	rm *.o
