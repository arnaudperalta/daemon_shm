#ifndef THREADMANAG__H
#define THREADMANAG__H

#include <stddef.h>

// Déclaration de la structure thread_m stockant les pointeurs de threads
//  avec leur sémaphores respectif
typedef struct thread_m thread_m;

// Procédure d'initialisation des threads, les paramètres indiquent le
//   le nombre de threads a initialisé et le nombre de commande a executer.
//	 On transmet un segment pour y initiliasé les sémaphores mutex de chaque thread.
extern thread_m *ini_thread(size_t min_thread, size_t max_con
    , size_t max_thread, size_t shm_size);

extern void *waiting_command(void *arg);

extern int use_thread(thread_m *th);

extern int consume_thread(thread_m *th, size_t number);

extern int fork_execute(char *command, size_t size);

extern void lock_thread(thread_m *th, size_t n);

extern void unlock_thread(thread_m *th, size_t n);

extern void thread_dispose(thread_m *th);

#endif
