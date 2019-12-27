CC = gcc
CFLAGS = -std=c11 -g -Wall -Wconversion -Werror -Wextra -Wpedantic
LDFLAGS = -lrt
VPATH =
objects = cpp.o
executable = cpp

all: $(executable)

clean:
	$(RM) $(objects) $(executable)

$(executable): $(objects)
	$(CC) $(objects) $(LDFLAGS) -o $(executable)

cpp.o: cpp.c
