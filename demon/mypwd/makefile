CC = gcc
CFLAGS = -std=c11 -g -Wall -Wconversion -Werror -Wextra -Wpedantic
LDFLAGS = -lrt
VPATH =
objects = mypwd.o
executable = mypwd

all: $(executable)

clean:
	$(RM) $(objects) $(executable)

$(executable): $(objects)
	$(CC) $(objects) $(LDFLAGS) -o $(executable)

mypwd.o: mypwd.c
