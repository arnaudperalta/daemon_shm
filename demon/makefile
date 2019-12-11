thread_managerdir = thread_manager/
defsdir = ../

CC = gcc
CFLAGS = -std=c11 -D_XOPEN_SOURCE=500 -Wall -Wconversion -Werror -Wextra -Wpedantic -I$(defsdir) -I$(thread_managerdir)
LDFLAGS = -lrt -pthread
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
