CC = gcc
CFLAGS = -Wall -O2 -pthread
OBJ = main.o mapreduce.o datastructs.o

.PHONY: all build clean

all: tema1

build: all

tema1: $(OBJ)
	$(CC) $(CFLAGS) -o tema1 $(OBJ)

main.o: main.c mapreduce.h datastructs.h
	$(CC) $(CFLAGS) -c main.c

mapreduce.o: mapreduce.c mapreduce.h datastructs.h
	$(CC) $(CFLAGS) -c mapreduce.c

datastructs.o: datastructs.c datastructs.h
	$(CC) $(CFLAGS) -c datastructs.c

clean:
	rm -f $(OBJ) tema1
