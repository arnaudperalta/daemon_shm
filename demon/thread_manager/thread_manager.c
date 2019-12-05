#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <pthread.h>

#include "thread_manager.h"

// Structure element stockant une paire thread / semaphore / connection restante
typedef struct thread_e {
	pthread_t thread;
	pthread_mutex_t mutex;
	size_t con_left;
} thread_e;

// Structure principale stockant chaque paire thread/semaphore
struct thread_m {
	thread_e *array;
	size_t count;
} thread_m;

thread_e *ini_thread_element(size_t max_con) {
	thread_e ptr
	pthread_t th;
	ptr->thread = th;
	pthread_mutex_t mu;
	if (pthread_mutex_init(&mu, NULL) != 0) {
		perror("Error pthread mutex");
		return NULL;
	}
	ptr->mutex = mu;
	ptr->con_left = max_con;
	
	pthread_create(ptr->thread, NULL, waiting_command, ptr);
	return ptr;
}


thread_m *ini_thread(size_t count, size_t max_con, int shm_sem) {
	// Allocation de la structure thread_m
	pthread_m *ptr = malloc(sizeof (thread_m));
	if (ptr == NULL) {
		return NULL;
	}
	ptr->count = count;
	
	// Allocation du tableau de pointeur de thread_e
	ptr->array = malloc(sizeof (thread_e) * count);
	if (ptr->array == NULL) {
		return NULL
	}
	
	// Allocation de "count" threads
	for (int i = 0; i < count; ++i) {
		ptr->array[i] = ini_thread_element(max_con);
	}
	
	return ptr;
}

void *waiting_command(void *arg) {
	thread_e *ptr = (thread_e *) arg;
	// Lecture d'une commande
	
	// Execution
	pthread_mutex_lock(ptr->mutex);
	
	pthread_mutex_unlock(ptr->mutex);
	return NULL;
}
