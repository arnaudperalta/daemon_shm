thread_managerdir = thread_manager/
defsdir = ../

CC = gcc
CFLAGS = -std=c11 -Wall -Wconversion -Werror -Wextra -Wpedantic -pthread -I$(defsdir) -I$(thread_managerdir)
LDFLAGS = -lrt
VPATH = $(thread_managerdir) $(defsdir)
objects = demon.o $(thread_managerdir)thread_manager.o
executable = daemon_shm

all: $(executable)

clean:
	$(RM) $(objects) $(executable)

$(executable): $(objects)
	$(CC) $(objects) $(LDFLAGS) -o $(executable)

$(thread_managerdir)thread_manager.o: thread_manager.c thread_manager.h defs.h
demon.o: demon.c demon.h thread_manager.h defs.h
