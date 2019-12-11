#ifndef THREADMANAG__H
#define THREADMANAG__H

#include <stddef.h>

// Déclaration de la structure thread_m stockant les pointeurs de threads
//  avec leur sémaphores respectif
typedef struct thread_m thread_m;

// Procédure d'initialisation des threads, les paramètres indiquent le
//   le nombre de threads a initialisé et le nombre de commande a executer.
//	 On transmet un segment pour y initiliasé les sémaphores mutex de chaque thread.
extern thread_m *ini_thread(size_t min_thread, size_t max_con, size_t max_thread);

extern void *waiting_command(void *arg);

extern int use_thread(thread_m *th, size_t max_con);

#endif
