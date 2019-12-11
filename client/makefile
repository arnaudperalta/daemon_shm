defsdir = ../

CC = gcc
CFLAGS = -std=c11 -Wall -Wconversion -Werror -Wextra -Wpedantic -pthread -I$(defsdir)
LDFLAGS = -lrt
VPATH = $(defsdir)
objects = client.o
executable = client_shm

all: $(executable)

clean:
	$(RM) $(objects) $(executable)

$(executable): $(objects)
	$(CC) $(objects) $(LDFLAGS) -o $(executable)

client.o: client.c client.h defs.h
