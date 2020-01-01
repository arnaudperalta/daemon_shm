defsdir = ../

CC = gcc
CFLAGS = -std=c11 -g -Wall -Wconversion -Werror -Wextra -Wpedantic -pthread -I$(defsdir)
LDFLAGS = -lrt
VPATH = $(defsdir)
objects = client.o
executable = client

all: $(executable)

clean:
	$(RM) $(objects) $(executable)

$(executable): $(objects)
	$(CC) $(objects) $(LDFLAGS) -o $(executable)

client.o: client.c defs.h
