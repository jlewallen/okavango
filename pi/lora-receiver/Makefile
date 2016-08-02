CC=g++
CFLAGS=-c -Wall -ggdb
LIBS=-lwiringPi -lpthread

all: lora-receiver

lora-receiver.o: lora-receiver.cpp
	$(CC) $(CFLAGS) lora-receiver.cpp

lora-receiver: lora-receiver.o
	$(CC) lora-receiver.o $(LIBS) -o lora-receiver

clean:
	rm -f lora-receiver lora-receiver.o

run: lora-receiver
	sudo ./lora-receiver
