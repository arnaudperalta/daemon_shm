thread_managerdir = thread_manager/
defsdir = ../

CC = gcc
CFLAGS = -std=c11 -g -Wall -Wconversion -Werror -Wextra -Wfatal-errors -Wpedantic -O2 -I$(defsdir) -I$(thread_managerdir) -D_XOPEN_SOURCE=500
LDFLAGS = -pthread -lrt 
VPATH = $(thread_managerdir) $(defsdir)
objects = demon.o $(thread_managerdir)thread_manager.o
executable = demon

all: $(executable)

clean:
	$(RM) $(objects) $(executable)

$(executable): $(objects)
	$(CC) $(objects) $(LDFLAGS) -o $(executable)

$(thread_managerdir)thread_manager.o: thread_manager.c thread_manager.h defs.h
demon.o: demon.c thread_manager.h defs.h
