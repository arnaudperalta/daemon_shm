#ifndef THREADMANAG__H
#define THREADMANAG__H

#include <stddef.h>

// Déclaration de la structure thread_m stockant les pointeurs de threads
//  avec leur sémaphores respectif
typedef struct thread_m thread_m;

// Procédure d'initialisation des threads, les paramètres indiquent le
//   le nombre de threads a initialisé et le nombre de commande a executer.
//	 On transmet un segment pour y initiliasé les sémaphores mutex de chaque thread.
extern thread_m *ini_thread(size_t count, size_t max_con, int shm_sem);

#endif