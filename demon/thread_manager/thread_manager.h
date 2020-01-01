#ifndef THREADMANAG__H
#define THREADMANAG__H

// Déclaration de la structure thread_m stockant les pointeurs de threads
//  avec leur mutex et informations respectives.
typedef struct thread_m thread_m;

// Procédure d'initialisation des threads, les paramètres indiquent le
//   le nombre de threads a initialisé, le nombre de thread maximal, 
//    le nombre de connexion maximal pour chaque thread, 
//    le nombre de commande a executer et la taille de la shm
extern thread_m *ini_thread(size_t min_thread, size_t max_con
    , size_t max_thread, size_t shm_size);

// Fonction qui distribue un thread a utilisé pour un nouveau client
extern int use_thread(thread_m *th);

// Fonction qui consume un thread d'une charge après la déconnection
// d'un client
extern int consume_thread(thread_m *th, size_t number);

// Fonction de nettoyage de la structure thread_m
extern void thread_dispose(thread_m *th);

#endif
