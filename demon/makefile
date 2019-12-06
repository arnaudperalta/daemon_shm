thread_managerdir = /thread_manager/

CC = gcc
CFLAGS = -std=c11 -Wall -Wconversion -Werror -Wextra -Wpedantic -pthread -I$(thread_managerdir)
LDFLAGS =
VPATH = $(thread_managerdir)
objects = demon.o $(thread_managerdir)thread_manager.o
executable = daemon_shm

all: $(executable)

clean:
	$(RM) $(objects) $(executable)

$(executable): $(objects)
	$(CC) $(objects) $(LDFLAGS) -o $(executable)

demon.o: demon.c demon.h thread_manager.h
thread_manager.o: thread_manager.c thread_manager.h
