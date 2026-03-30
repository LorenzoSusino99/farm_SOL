CC = gcc
CFLAGS = -Wall -Wextra -pthread

OBJ = main.o master.o worker.o collector.o queue.o util.o

farm : $(OBJ)
	$(CC) $^ -o $@ $(CFLAGS)
	
main.o : main.c farm.h
	$(CC) ${CFLAGS } -c main.c
master.o : master.c farm.h master.h
	$(CC) ${CFLAGS } -c master.c
worker.o : worker.c farm.h worker.h
	$(CC) ${CFLAGS } -c worker.c
collector.o : collector.c farm.h collector.h
	$(CC) ${CFLAGS } -c collector.c
queue.o : queue.c farm.h queue.h
	$(CC) ${CFLAGS } -c queue.c 
util.o : util.c farm.h util.h
	$(CC) ${CFLAGS } -c util.c
clean :
	rm farm $(OBJ)



